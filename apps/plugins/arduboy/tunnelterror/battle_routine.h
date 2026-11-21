// This is the entire battle routine
// dice_roll, player_turn, alien_turn, fight_alien

int dice_roll(int8_t X, int8_t Y)
{
    int8_t dice_val;
    arduboy.initRandomSeed();
    for (int8_t i = 1; i <= 12; i++)
    {
        arduboy.clear();
        dice_val = random(1, 7);
        draw_dice(X, Y, dice_val);
        arduboy.display();
        delay(i * 10 + 50);
    }
    return (dice_val);
}

int player_turn()
{
    arduboy.clear();
    arduboy.print(F("Roll your first dice"));
    waitForButtonAPress();
    int player_dice_one = dice_roll(MIDDLE.x - 15, MIDDLE.y);
    arduboy.setCursor(0, 0);
    arduboy.print(F("\nA - Roll next dice\n"));
    arduboy.print(F("B - Block Attack"));
    arduboy.display();
    if (waitForButtonPress()==B_BUTTON){
        if (stamina>0)
            return 0;
        else {
            arduboy.clear();
            arduboy.print(F("Not enough Health\nto Block Attack"));
            waitForButtonAPress();
        }
    }
    //player wants to Attack
    int8_t player_dice_two = dice_roll(MIDDLE.x - 15, MIDDLE.y);
    arduboy.setCursor(0, 0);
    arduboy.print(F("You Scored:"));
    arduboy.print(player_dice_one + player_dice_two);
    waitForButtonAPress();
    return (player_dice_one + player_dice_two);
}

int alien_turn(alien theAlien)
{
    arduboy.clear();
    arduboy.print(F("Alien Turn!"));
    arduboy.display();
    delay(1600);
    int8_t alien_dice_one = dice_roll(MIDDLE.x - 15, MIDDLE.y);
    if (alien_dice_one < 4 && random(1, 3) == 2 && theAlien.stamina>0)
    {
        //they decide to block
        return 0;
    }
    //they want to Attack
    arduboy.setCursor(0, 0);
    arduboy.print(F("Alien Next Throw"));
    arduboy.display();
    delay(1500);
    int8_t alien_dice_two = dice_roll(MIDDLE.x - 15, MIDDLE.y);
    arduboy.setCursor(0, 0);
    arduboy.print(F("Alien Scored:"));
    arduboy.print(alien_dice_one + alien_dice_two);
    waitForButtonAPress();
    return (alien_dice_one + alien_dice_two);
}

void fight_alien(alien theAlien)
{
    arduboy.initRandomSeed();
    if (random(1,5)==1){
        arduboy.clear();
        arduboy.print(F("Alien ran away!\nLucky escape!"));
        waitForButtonAPress();
        return;
    }
    int8_t player_dice_total, alien_dice_total, button_pressed;
    int8_t alien_start_strength = theAlien.strength;
    bool button_press=false;
    arduboy.clear();
    arduboy.print(F("Your Str:"));
    arduboy.print(strength);
    arduboy.print(F(" Sta:"));
    arduboy.print(stamina);
    arduboy.print(F("\nAlien Str:"));
    arduboy.print(theAlien.strength);
    arduboy.print(F(" Sta:"));
    arduboy.print(theAlien.stamina);
    if (dbl_power)
        arduboy.print(F("\nYou have 2x Power"));
    waitForButtonAPress();
    if (invincibility){
        //they have invincibility potion so see if they want to use it
        arduboy.clear();
        print_top_bar();
        arduboy.setCursor(0, 10);
        arduboy.print(F("You have invincible\nDo you want\nto use it?\n"));
        arduboy.print(F("\nA = YES\nB = NO"));
        arduboy.display();
        button_pressed = waitForButtonPress();
        if (button_pressed==A_BUTTON)
        {
            invincibility=false;
            theAlien.strength=0;
            button_press=true;
        }

    }
    //keep fighting until someone's strength = zero
    while (strength > 0 && theAlien.strength > 0)
    {
        player_dice_total = player_turn();
        if (player_dice_total == 0)
        {
            //the player blocked
            stamina--;
            if (stamina < 0)
            {
                strength--;
                stamina = 0;
            }
            arduboy.clear();
            print_top_bar();
            arduboy.setCursor(0, 16);
            arduboy.print(F("You Blocked!\nLose Stamina Point"));
            waitForButtonAPress();
            continue; //jump to next fight iteration
        }

        int8_t alien_dice_total = alien_turn(theAlien);
        if (alien_dice_total == 0)
        {
            //the alein blocked
            theAlien.stamina--;
            if (theAlien.stamina < 0)
            {
                theAlien.strength--;
                theAlien.stamina = 0;
            }
            arduboy.clear();
            print_top_bar();
            arduboy.setCursor(0, 10);
            arduboy.print(F("Alien Blocked!\n\nYour Strength:"));
            arduboy.print(strength);
            arduboy.print(F("\nAlien Strength:"));
            arduboy.print(theAlien.strength);
            waitForButtonAPress();
            continue; //jump to next fight iteration
            }
        arduboy.clear();
        arduboy.setCursor(0, 16);
        arduboy.print(F("You Scored:"));
        arduboy.print(player_dice_total+stamina);
        arduboy.print(F("\nAlien Scored:"));
        arduboy.print(alien_dice_total+theAlien.stamina);
        if ((player_dice_total+stamina) > (alien_dice_total+theAlien.stamina))
        {
            sound.tone(NOTE_C5,150,NOTE_C5,600);
            arduboy.print(F("\nYOU WIN ROUND!"));
            theAlien.strength--;
            if(dbl_power)
                theAlien.strength--; //if player has double power, alien loses another point
        }
        else if(player_dice_total+stamina < alien_dice_total+theAlien.stamina)
        {
            sound.tone(NOTE_C1,800);
            arduboy.print(F("\nYou Lose Round"));
            strength--;
        }
        else if(player_dice_total+stamina==alien_dice_total+theAlien.stamina){
            arduboy.print(F("\nDraw!"));
            stamina--;
            if (stamina<0){
                strength--;
                stamina=0;
            }
            theAlien.stamina--;
            if (theAlien.stamina<0){
                theAlien.strength--;
                theAlien.stamina=0;
            }
        }
        arduboy.print(F("\nAlien Strength:"));
        arduboy.print(theAlien.strength);
        arduboy.print(F("\nAlien Health:"));
        arduboy.print(theAlien.stamina);
        print_top_bar();
        waitForButtonAPress();
    } //end of while loop

    if (dbl_power)
        dbl_power=false;
    //now work out who lost a life from the battle
    if (strength <= 0)
    {
        //player lost
        sound.tone(NOTE_C4,500, NOTE_C3,500, NOTE_C1,1000);
        lives--;
        arduboy.clear();
        arduboy.print(F("You lost battle\nlose a life!"));
        strength=1; stamina=1;
        waitForButtonAPress();
    }
    else
    {
        //alien lost you gain the aliens strength
        sound.tone(NOTE_C4,150, NOTE_C5,150, NOTE_C6,800);
        strength += alien_start_strength;
        if (alien_start_strength > 2)
            strength += (alien_start_strength / 2);
        if (stamina<=1)
            stamina++;
        if (strength>10)
            strength=10;
        arduboy.clear();
        arduboy.print(F("Alien is DEAD!!\nYour new strength="));
        arduboy.print(strength);
        arduboy.print(F("\nYour new health="));
        arduboy.print(stamina);
        waitForButtonAPress();
    }
}
