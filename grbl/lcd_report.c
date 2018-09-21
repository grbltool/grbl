/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "grbl.h"


int8 fro;
int8 lcd_screen = MAIN_SCREEN;
int8 menu_item_selected = false;
int8 lcd_cursor = 4;
int8 lcd_cursor_max = 6;
int8 switch_clicked = false;
int16 lcd_FRO = 100; 
int16 lcd_SRO = 100;
// TO DO store these in EEPROM 
float lcd_probe_thickness = 15.0;
int16 lcd_probe_rate = 100;
int16 lcd_probe_dist = 120;

static uint8 led_state = true;

void lcd_init()
{

    LED_CONTROL_REG_Write(led_state);

    I2C_LCD_4x20_Start();
  
    LCD_4x20_Start();
    LCD_4x20_LoadCustomFonts(LCD_4x20_customFonts); 
    LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
    //ISR_LCD_UPDATE_StartEx(isr_lcd_update);
    
    QUAD_DECODER_Start();
    //ISR_QUAD_DECODER_SWITCH_StartEx(isr_quad_decoder_switch_handler);
}

CY_ISR(isr_quad_decoder_switch_handler)
{
  switch_clicked = true;
}

void lcd_report_init_message()
{
    LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
    LCD_4x20_Position(0u, 3u);
    
    /* Output demo start message */
    LCD_4x20_PrintString(GRBL_PORT " " GRBL_VERSION);
    delay_ms(1000);
    LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
    
    lcd_report_realtime();
    
}

CY_ISR(isr_lcd_update)
{
  
  led_state = led_state ? false : true;
  LED_CONTROL_REG_Write(led_state);
  // Execute and LCD print status
  lcd_report_realtime(); 
}

void get_state(char *foo)
{        
    // pad them to same length
    switch (sys.state) {
    case STATE_IDLE: strcpy(foo,"Idle ");; break;
    case STATE_CYCLE: strcpy(foo,"Run  "); break;
    case STATE_HOLD: strcpy(foo,"Hold "); break;
    case STATE_HOMING: strcpy(foo,"Home "); break;
    case STATE_ALARM: strcpy(foo,"Alarm"); break;
    case STATE_CHECK_MODE: strcpy(foo,"Check"); break;
    case STATE_SAFETY_DOOR: strcpy(foo,"Door "); break;
    default:strcpy(foo," ?  "); break;
  }     
}

void print_coord ( char* buf, float value ) {

  char *sign = (value < 0) ? "-" : "+";
  float tmp = (value < 0) ? -value : value;

  int i1 = tmp;
  float frac = tmp - i1;
  int i2 = trunc(frac * 1000);  // 2 decimals

  // Print as parts, note that you need 0-padding for fractional bit.

  sprintf (buf, "%s%d.%03d", sign, i1, i2);
  
}

void lcd_report_realtime()
{
  uint8_t idx;
  char stat[12];    
  char fstr[20];
  char line[20];

  switch (lcd_screen)
  {
    case MAIN_SCREEN:   // ====================================================
      // Current machine state
      lcd_cursor_max = 7;
      get_state(stat);
      LCD_4x20_Position(0u, 1u);
      LCD_4x20_PrintString(stat);

#ifdef XYZ      
      if (switch_clicked) {
        switch_clicked = false;
        switch (lcd_cursor)
        {
          case MENU_ITEM_MAIN_STATUS:
            if (sys.state == STATE_ALARM) {  // Ignore if already in alarm state. 
              system_execute_line("$X");
            }
          break;
          case MENU_ITEM_MAIN_X_DRO:
           lcd_menu_execute("G10L20P0X0");
          break;
          case MENU_ITEM_MAIN_Y_DRO:
           lcd_menu_execute("G10L20P0Y0");
          break;
          case MENU_ITEM_MAIN_Z_DRO:
           lcd_menu_execute("G10L20P0Z0");
          break;
          
          case MENU_ITEM_MAIN_UNIT:
            if (lcd_units == LCD_UNIT_MM)
              lcd_units = LCD_UNIT_IN;
            else
              lcd_units = LCD_UNIT_MM;
          break;
          case MENU_ITEM_MAIN_FRO:
              menu_item_selected = !menu_item_selected;
              if (menu_item_selected)
                QUAD_DECODER_SetCounter(lcd_FRO);
              else
                QUAD_DECODER_SetCounter(MENU_ITEM_MAIN_FRO);
          break;  
          case MENU_ITEM_MAIN_SRO:
              menu_item_selected = !menu_item_selected;
              if (menu_item_selected)
                QUAD_DECODER_SetCounter(lcd_SRO);
              else
                QUAD_DECODER_SetCounter(MENU_ITEM_MAIN_SRO);
          break;     
          case MENU_ITEM_MAIN_MORE:
            lcd_screen = CMD_SCREEN; 
            lcd_cursor = 0;
            QUAD_DECODER_SetCounter(lcd_cursor);
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
          break;
            
        }
        if (lcd_screen != MAIN_SCREEN) break;       
      }
#endif      

#ifdef XYZ
      if (switches) {
        LCD_4x20_Position(1u, 0u);
        LCD_4x20_PrintString("Limit:");
        lcd_unsigned_int8(LIMIT_STATUS_REG_Read(), 2, 3);      
        
        LCD_4x20_Position(2u, 0u);
        LCD_4x20_PrintString("Ctl:");    
        lcd_unsigned_int8(CONTROL_STATUS_REG_Read(), 2, 4);
        
        LCD_4x20_Position(3u, 0u);
        LCD_4x20_PrintString("Probe:");
        lcd_unsigned_int8(PROBE_STATUS_REG_Read(), 2, 1);
        break;
      }
#endif    

      int32_t current_position[N_AXIS]; // Copy current state of the system position variable
      memcpy(current_position,sys_position,sizeof(sys_position));
      float print_position[N_AXIS];
      system_convert_array_steps_to_mpos(print_position,current_position);
      float wco[N_AXIS];
      for (idx=0; idx< N_AXIS; idx++) {
        // Apply work coordinate offsets and tool length offset to current position.
        wco[idx] = gc_state.coord_system[idx]+gc_state.coord_offset[idx];
        if (idx == TOOL_LENGTH_OFFSET_AXIS) { wco[idx] += gc_state.tool_length_offset; }
        print_position[idx] -= wco[idx];
      }

      char buf [20];
      
      LCD_4x20_Position(1u, 1u);
      LCD_4x20_PrintString("X");
      print_coord ( buf, print_position[X_AXIS] );
      sprintf(fstr, "%8s", buf ); 
      LCD_4x20_PrintString(fstr);    
    
      LCD_4x20_Position(2u, 1u);
      LCD_4x20_PrintString("Y");
      print_coord ( buf, print_position[Y_AXIS] );
      sprintf(fstr, "%8s", buf ); 
      LCD_4x20_PrintString(fstr);      
      
      LCD_4x20_Position(3u, 1u);
      LCD_4x20_PrintString("Z");
      print_coord ( buf, print_position[Z_AXIS] );
      sprintf(fstr, "%8s", buf ); 
      LCD_4x20_PrintString(fstr);      
      
#ifdef XYZ
      LCD_4x20_Position(1u, 12u);
      LCD_4x20_PrintString("FRO:");
      sprintf(fstr, "%3d", lcd_FRO ); 
      LCD_4x20_PrintString(fstr);
      LCD_4x20_PrintString("%");
      
      LCD_4x20_Position(2u, 12u);
      LCD_4x20_PrintString("SRO:");
      sprintf(fstr, "%3d", lcd_SRO ); 
      LCD_4x20_PrintString(fstr);
      LCD_4x20_PrintString("%");
#endif      
      
      LCD_4x20_Position(3u, 12u);
      LCD_4x20_PrintString("More ");
      //LCD_4x20_PutChar(LCD_4x20_CUSTOM_2);

#ifdef XYZ
      if (menu_item_selected) {
        switch (lcd_cursor) {
         case MENU_ITEM_MAIN_FRO:
          lcd_FRO = QUAD_DECODER_GetCounter();
          if (lcd_FRO > LCD_FRO_MAX) lcd_FRO = LCD_FRO_MAX;
          if (lcd_FRO < LCD_FRO_MIN) lcd_FRO = LCD_FRO_MIN;
         break; 
          case MENU_ITEM_MAIN_SRO:
          lcd_SRO = QUAD_DECODER_GetCounter();
          if (lcd_SRO > LCD_SRO_MAX) lcd_FRO = LCD_SRO_MAX;
          if (lcd_SRO < LCD_SRO_MIN) lcd_FRO = LCD_SRO_MIN;
          break;
        }
      }
      else {
        lcd_cursor = QUAD_DECODER_GetCounter();
        if  (lcd_cursor > lcd_cursor_max) lcd_cursor = lcd_cursor_max;
        if  (lcd_cursor < 0) lcd_cursor = 0;
        
        QUAD_DECODER_SetCounter(lcd_cursor);
        
      }
#endif      
       
      
      LCD_4x20_WriteControl(LCD_4x20_DISPLAY_ON_CURSOR_OFF);
      
#ifdef XYZ      
      switch (lcd_cursor) {
       case MENU_ITEM_MAIN_STATUS:
        LCD_4x20_Position(0u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case MENU_ITEM_MAIN_X_DRO:
        LCD_4x20_Position(1u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case MENU_ITEM_MAIN_Y_DRO:
        LCD_4x20_Position(2u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break; 
       case MENU_ITEM_MAIN_Z_DRO:
        LCD_4x20_Position(3u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case MENU_ITEM_MAIN_UNIT:
        LCD_4x20_Position(0u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case MENU_ITEM_MAIN_FRO:
        LCD_4x20_Position(1u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
        if (menu_item_selected) {
          LCD_4x20_Position(1u, 11u);
          LCD_4x20_WriteControl(LCD_4x20_CURSOR_WINK);
        }
       break;
       case MENU_ITEM_MAIN_SRO:
        LCD_4x20_Position(2u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
        if (menu_item_selected) {
          LCD_4x20_Position(2u, 11u);
          LCD_4x20_WriteControl(LCD_4x20_CURSOR_WINK);
        }
       break;
       case MENU_ITEM_MAIN_MORE:
        LCD_4x20_Position(3u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;       
      }
#endif      
    break; // main screen

#ifdef XYZ      
    case CMD_SCREEN:  // ============================================================
      lcd_cursor_max = 7;
      
      if (switch_clicked) {
        switch_clicked = false;
        switch (lcd_cursor)
        {
          case 0: // return to main
            lcd_screen = MAIN_SCREEN; 
            lcd_cursor = 0;        
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
          break;
            
          case 1: // home axes
            if (bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) { 
            // Block if safety door is ajar.
            if (system_check_safety_door_ajar()) { lcd_beep(); }
            sys.state = STATE_HOMING; // Set system state variable
            mc_homing_cycle(HOMING_CYCLE_ALL);
            if (!sys.abort) {  // Execute startup scripts after successful homing.
              sys.state = STATE_IDLE; // Set to IDLE when complete.
              st_go_idle(); // Set steppers to the settings idle state before returning.
              //system_execute_startup(line); 
            }
          } else { lcd_beep(); }
          break;
            
          case 2: // Top of Z
            lcd_menu_execute("G53G0Z-1");
          break;
            
          case 3: // go to probe screen
            lcd_screen = PROBE_SCREEN;
            lcd_cursor_max = 4;
            lcd_cursor = 0;
            QUAD_DECODER_SetCounter(lcd_cursor);
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
          break;  
            
          case 4:  // goto 0,0
            lcd_menu_execute("G0X0Y0");
          break;
            
          case 5: // G28
            lcd_menu_execute("G28");
          break;
            
          case 6: // G30
            lcd_menu_execute("G30");
          break;
            
          case 7: // sd card
            lcd_screen = SD_CARD_SCREEN; 
            lcd_cursor = 0;        
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
          break;  
            
        }
        
        if (lcd_screen != CMD_SCREEN) break;
      }
    
      LCD_4x20_Position(0u, 0u);
      LCD_4x20_PrintString(" Back ");
      LCD_4x20_PutChar(LCD_4x20_CUSTOM_3);
      LCD_4x20_Position(1u, 0u);
      LCD_4x20_PrintString(" Home Axes");
      LCD_4x20_Position(2u, 0u);
      LCD_4x20_PrintString(" Top of Z");
      LCD_4x20_Position(3u, 0u);
      LCD_4x20_PrintString(" Z Probe ");
      LCD_4x20_PutChar(LCD_4x20_CUSTOM_2);
      
      LCD_4x20_Position(0u, 11u);
      LCD_4x20_PrintString(" Goto 0,0");
      LCD_4x20_Position(1u, 11u);
      LCD_4x20_PrintString(" Goto G28");
      LCD_4x20_Position(2u, 11u);
      LCD_4x20_PrintString(" Goto G30");
      LCD_4x20_Position(3u, 11u);
      LCD_4x20_PrintString(" SdCard ");
      LCD_4x20_PutChar(LCD_4x20_CUSTOM_2);
        
      if (menu_item_selected) {
      }
      else {
        lcd_cursor = QUAD_DECODER_GetCounter();
        if  (lcd_cursor > lcd_cursor_max) lcd_cursor = lcd_cursor_max;
        if  (lcd_cursor < 0) lcd_cursor = 0;
        
        QUAD_DECODER_SetCounter(lcd_cursor);
        
      }
        
      switch (lcd_cursor)
      {
       case 0:
        LCD_4x20_Position(0u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 1:
        LCD_4x20_Position(1u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break; 
       case 2:
        LCD_4x20_Position(2u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 3:
        LCD_4x20_Position(3u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 4:
        LCD_4x20_Position(0u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 5:
        LCD_4x20_Position(1u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 6:
        LCD_4x20_Position(2u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
        case 7:
        LCD_4x20_Position(3u, 11u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break; 
      }
        
    break;
      
    case PROBE_SCREEN: // ===============================================================
      
      if (switch_clicked) {
        switch_clicked = false;
        switch (lcd_cursor)
        {
          case MENU_ITEM_PRB_BACK: // return to main
            lcd_screen = CMD_SCREEN;            
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
          break;
            
          case MENU_ITEM_PRB_PROBE:
            LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
            lcd_screen = MAIN_SCREEN;
            strcpy(line, "G91G38.2Z-");
            sprintf(fstr, "%d", lcd_probe_dist);
            strcat(line, fstr);
            strcat(line, "F");
            sprintf(fstr, "%d", lcd_probe_rate);
            strcat(line, fstr);
            gc_execute_line(line);
            LCD_4x20_Position(3u, 0u);
            if (sys.probe_succeeded == false) {
              // TO DO Not sure if this works
              LCD_4x20_PrintString("Failure");
            }
            else {
              //LCD_4x20_Position(3u, 0u);
              LCD_4x20_PrintString("Success");
              sprintf(fstr, "%2.2f", lcd_probe_thickness);
              strcpy(line, "G10L20P0Z");
              strcat(line, fstr);
              gc_execute_line(line);
              
            }
          break;  
            
          case MENU_ITEM_PRB_THICK:
            menu_item_selected = !menu_item_selected;
            if (menu_item_selected)
                QUAD_DECODER_SetCounter(lcd_probe_thickness * LCD_PROBE_MULT);
              else
                QUAD_DECODER_SetCounter(lcd_cursor);
          break;  
                
          case MENU_ITEM_PRB_RATE:
                menu_item_selected = !menu_item_selected;
            if (menu_item_selected)
                QUAD_DECODER_SetCounter(lcd_probe_rate);
              else
                QUAD_DECODER_SetCounter(lcd_cursor);
          break;
                
          case MENU_ITEM_PRB_DIST:
                menu_item_selected = !menu_item_selected;
            if (menu_item_selected)
                QUAD_DECODER_SetCounter(lcd_probe_dist);
              else
                QUAD_DECODER_SetCounter(lcd_cursor);
          break;            
        }        
        if (lcd_screen != PROBE_SCREEN) break;
      }
      
      LCD_4x20_Position(0u, 0u);
      LCD_4x20_PrintString(" Back ");
      LCD_4x20_PutChar(LCD_4x20_CUSTOM_3);
      
      LCD_4x20_Position(0u, 8u);
      LCD_4x20_PrintString(" Begin Probe"); 
      
      LCD_4x20_Position(1u, 8u);
      LCD_4x20_PrintString(" Thick:");
      sprintf(fstr, "%2.2f", lcd_probe_thickness ); 
      LCD_4x20_PrintString(fstr);
      
      LCD_4x20_Position(2u, 8u);
      LCD_4x20_PrintString(" Rate:");
      sprintf(fstr, "%6d", lcd_probe_rate ); 
      LCD_4x20_PrintString(fstr);
      
      LCD_4x20_Position(3u, 8u);
      LCD_4x20_PrintString(" Dist:");
      sprintf(fstr, "%6d", lcd_probe_dist ); 
      LCD_4x20_PrintString(fstr);
     
      if (menu_item_selected) {
      }
      else {
        lcd_cursor = QUAD_DECODER_GetCounter();
        if  (lcd_cursor > lcd_cursor_max) lcd_cursor = lcd_cursor_max;
        if  (lcd_cursor < 0) lcd_cursor = 0;
        
        QUAD_DECODER_SetCounter(lcd_cursor);
        
      }
      
      if (menu_item_selected) {
        switch (lcd_cursor) {
         case MENU_ITEM_PRB_THICK:
          lcd_probe_thickness = (float)QUAD_DECODER_GetCounter() / LCD_PROBE_MULT;
          if (lcd_probe_thickness > LCD_PRB_THICK_MAX) lcd_probe_thickness = LCD_PRB_THICK_MAX;
          if (lcd_probe_thickness < LCD_PRB_THICK_MIN) lcd_probe_thickness = LCD_PRB_THICK_MIN;
          QUAD_DECODER_SetCounter(lcd_probe_thickness * LCD_PROBE_MULT);
         break;
         case MENU_ITEM_PRB_RATE:
          lcd_probe_rate = QUAD_DECODER_GetCounter();
          if (lcd_probe_rate > LCD_PRB_RATE_MAX) lcd_probe_rate = LCD_PRB_RATE_MAX;
          if (lcd_probe_rate < LCD_PRB_RATE_MIN) lcd_probe_rate = LCD_PRB_RATE_MIN;
          QUAD_DECODER_SetCounter(lcd_probe_rate);
         break;
         case MENU_ITEM_PRB_DIST:
          lcd_probe_dist = QUAD_DECODER_GetCounter();
          if (lcd_probe_dist > LCD_PRB_DIST_MAX) lcd_probe_dist = LCD_PRB_DIST_MAX;
          if (lcd_probe_dist < LCD_PRB_DIST_MIN) lcd_probe_dist = LCD_PRB_DIST_MIN;
          QUAD_DECODER_SetCounter(lcd_probe_dist);
         break; 
          
        }
      }
      else {
        lcd_cursor = QUAD_DECODER_GetCounter();
        if  (lcd_cursor > lcd_cursor_max) lcd_cursor = lcd_cursor_max;
        if  (lcd_cursor < 0) lcd_cursor = 0;
        
        QUAD_DECODER_SetCounter(lcd_cursor);        
      }
      
      switch (lcd_cursor)
      {
       case 0:
        LCD_4x20_Position(0u, 0u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 1:
        LCD_4x20_Position(0u, 8u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break; 
       case 2:
        LCD_4x20_Position(1u, 8u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 3:
        LCD_4x20_Position(2u, 8u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;
       case 4:
        LCD_4x20_Position(3u, 8u);
        LCD_4x20_PutChar(LCD_4x20_CUSTOM_0);
       break;       
      }
      
    break;  
      
    case SD_CARD_SCREEN:
      LCD_4x20_Position(0u, 0u);
      LCD_4x20_PrintString(" Back ");
      LCD_4x20_PutChar(LCD_4x20_CUSTOM_3);
      
      LCD_4x20_Position(2u, 0u);
      LCD_4x20_PrintString("Feature not ready"); 
      
      if (switch_clicked) {
        
        lcd_screen = CMD_SCREEN; 
        lcd_cursor = 0;
        QUAD_DECODER_SetCounter(lcd_cursor);
        LCD_4x20_WriteControl(LCD_4x20_CLEAR_DISPLAY);
        switch_clicked = false;
      }
      
    break;  
      
      
    default:
      LCD_4x20_Position(0u, 0u);
      sprintf(fstr, "Scn:%-5d", lcd_screen );
      LCD_4x20_PrintString(fstr);
    break;
#endif      
  }  // switch lcd_screen

    
}

// Prints an uint8 variable with base and number of desired digits.
void lcd_unsigned_int8(uint8_t n, uint8_t base, uint8_t digits)
{ 
  unsigned char buf[digits];
  uint8_t i = 0;

  for (; i < digits; i++) {
      buf[i] = n % base ;
      n /= base;
  }

  for (; i > 0; i--)
      LCD_4x20_PutChar('0' + buf[i - 1]);
}

void lcd_menu_execute(char *line)
{
    if ( sys.state == STATE_IDLE)
      gc_execute_line(line);
    else
      lcd_beep();
      
}

void lcd_beep()
{
}


