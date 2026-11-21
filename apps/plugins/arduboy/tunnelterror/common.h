void print_top_bar(){
  arduboy.setCursor(0,0);
  arduboy.print("S:");
  arduboy.print(strength);
  arduboy.print(" H:");
  arduboy.print(stamina);
  arduboy.print(" L:");
  arduboy.print(lives);
  arduboy.print(" C:");
  arduboy.print(chalices);
  if (invincibility){
    arduboy.print(" i");
  }
  if (dbl_power){
    arduboy.print(" 2");
  }
}

void waitForButtonAPress(){
  arduboy.print("\nPress A");
  arduboy.display();
  arduboy.waitNoButtons();
  do{
    delay(1);
  }while (!arduboy.pressed(A_BUTTON));
}

int8_t waitForButtonPress(){
    bool button_press=false;
    arduboy.waitNoButtons();
    do
    {
        delay(1);
        if (arduboy.pressed(A_BUTTON))
        {
            button_press = true;
            return A_BUTTON;
        }
        else if (arduboy.pressed(B_BUTTON))
        {
            button_press = true;
            return B_BUTTON;
        }
    } while (!button_press);
    return 0;
}
