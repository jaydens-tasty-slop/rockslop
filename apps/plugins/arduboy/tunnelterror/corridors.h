// Just one function to create the corridor pieces
void create_corridor()
{
  int8_t piece=0;
  stamina_counter++; // deplete stamina when moving more than 20 times
  if (stamina_counter==20){
    stamina--;
    if (stamina<0){
      strength--;
      stamina=0;
    }
    stamina_counter=0;
  }
  arduboy.clear();
  //generate random maze piece in front of player
  arduboy.initRandomSeed();
  piece = random(1, 19);
  switch (piece){
    case 1:
    case 2:
    case 3:
      //only straight on
      forward_allowed = true;
      right_allowed = false;
      left_allowed = false;
      door = false;
      arduboy.drawLine(0,0,WIDTH,HEIGHT,WHITE);
      arduboy.drawLine(WIDTH,0,0,HEIGHT,WHITE);
      break;
    case 4:
      //door
      forward_allowed = false;  
      right_allowed = false;
      left_allowed = false;
      door = true; 
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(0,0,DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(0,HEIGHT,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(WIDTH,0,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(WIDTH,HEIGHT,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      //now draw a door
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_LEFT_BOTTOM.x,DOOR_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,DOOR_RIGHT_BOTTOM.x,DOOR_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,WHITE);
      //now draw handle
      arduboy.drawCircle(DOOR_RIGHT_TOP.x-10,MIDDLE.y+6,2,WHITE);
      break;
    case 5:
    case 6:
      //right turn & straight on only
      forward_allowed = true;
      right_allowed = true;
      left_allowed = false;
      door = false;
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,0,HEIGHT,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,0,0,WHITE);
      arduboy.drawLine(WIDTH,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(WIDTH,DEPTH_RIGHT_BOTTOM.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      break;
    case 7:
    case 8:
      //left turn & straight on only
      forward_allowed = true;
      right_allowed = false;
      left_allowed = true;
      door = false;
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WIDTH,HEIGHT,WHITE);
      arduboy.drawLine(DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WIDTH,0,WHITE);
      arduboy.drawLine(0,DEPTH_LEFT_TOP.y,DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(0,DEPTH_LEFT_BOTTOM.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      break;
    case 9: case 10:
      //right & left turn & straight on available
      forward_allowed = true;
      right_allowed = true;
      left_allowed = true;
      door = false;
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,MIDDLE.x,MIDDLE.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,MIDDLE.x,MIDDLE.y,WHITE);
      arduboy.drawLine(0,DEPTH_LEFT_TOP.y,DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(0,DEPTH_LEFT_BOTTOM.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      //now other side
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,MIDDLE.x,MIDDLE.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,MIDDLE.x,MIDDLE.y,WHITE);
      arduboy.drawLine(WIDTH,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(WIDTH,DEPTH_RIGHT_BOTTOM.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      break;
    case 11: case 12: 
      //right only
      forward_allowed = false;
      right_allowed = true;
      left_allowed = false;
      door = false;
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WIDTH,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WIDTH,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(0,0,DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(0,HEIGHT,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      break;
    case 13: case 14:
      //left only
      forward_allowed = false;
      right_allowed = false;
      left_allowed = true;
      door = false;
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,0,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,0,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(WIDTH,0,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(WIDTH,HEIGHT,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      break;
    case 15: case 16:
      //door & left turn
      forward_allowed = false;
      right_allowed = false;
      left_allowed = true;
      door = true;
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,0,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,0,DEPTH_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(WIDTH,0,DEPTH_RIGHT_TOP.x,DEPTH_RIGHT_TOP.y,WHITE);
      arduboy.drawLine(WIDTH,HEIGHT,DEPTH_RIGHT_BOTTOM.x,DEPTH_RIGHT_BOTTOM.y,WHITE);
      //now draw a door
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_LEFT_BOTTOM.x,DOOR_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,DOOR_RIGHT_BOTTOM.x,DOOR_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,WHITE);
      //now draw handle
      arduboy.drawCircle(DOOR_RIGHT_TOP.x-10,MIDDLE.y+6,2,WHITE);
      break;
    case 17: case 18:
      //door & right turn
      forward_allowed = false;
      right_allowed = true;
      left_allowed = false;
      door = true;
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WIDTH,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WIDTH,DEPTH_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(0,0,DEPTH_LEFT_TOP.x,DEPTH_LEFT_TOP.y,WHITE);
      arduboy.drawLine(0,HEIGHT,DEPTH_LEFT_BOTTOM.x,DEPTH_LEFT_BOTTOM.y,WHITE);
      //now draw a door
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_LEFT_BOTTOM.x,DOOR_LEFT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,DOOR_RIGHT_BOTTOM.x,DOOR_RIGHT_BOTTOM.y,WHITE);
      arduboy.drawLine(DOOR_LEFT_TOP.x,DOOR_LEFT_TOP.y,DOOR_RIGHT_TOP.x,DOOR_RIGHT_TOP.y,WHITE);
      //now draw handle
      arduboy.drawCircle(DOOR_RIGHT_TOP.x-10,MIDDLE.y+6,2,WHITE);
      break;
    }
}