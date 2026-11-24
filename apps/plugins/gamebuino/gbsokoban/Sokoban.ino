// Sokoban (partie 4)
// Terminons cette série de tutos par le jeu complet
// Au programme: 
//           - ajout du joueur
//           - gestion des mouvements
//           - gestion des caisses mobiles
//           - ajout des fonctionnalités affichage de la mini carte et recommencer le niveau
//           - gestion des niveaux (changement de niveau et fin de partie)
//
// Source des niveaux: http://www.sourcecode.se/sokoban/levels
//


#include <Gamebuino-Meta.h>
#include "Level.h"
#include "Player.h"
#include "Graphics.h"
#include "Help.h"

enum class State : uint8_t {
  initialize,
  startmenu,
  help,
  running,
  endlevel,
  map,
  displaylegend,
  reinit,
  gameover
};

enum class SoundToPlay : uint8_t {
  MovePlayer,
  MoveBox,
  EndLevel,
  NewLevel,
  Nothing,
  EndGame
};

float decalageH;
float decalageV;
char pas;

struct Game {
  State state;
  uint32_t NIVEAU_COURRANT;
  uint8_t HELP_PAGE;
};

Game game;


SoundToPlay soundtoplay;

void playSound() {
  switch (soundtoplay) {
    case SoundToPlay::MovePlayer:
      gb.sound.tone(750,50);
      break;
    case SoundToPlay::MoveBox:
      gb.sound.tone(100,100);
      // gb.sound.playOK();
      break;
    case SoundToPlay::EndLevel:
      gb.sound.play("Sound/EndLevel.wav");
      break;
    case SoundToPlay::EndGame:
      gb.sound.play("Sound/Gameover.wav");
      break;  
    case SoundToPlay::Nothing: 
    default:
      break;
  }
  //une fois qu'on a joué le son demandé, on remet l'évènement à zéro
  soundtoplay = SoundToPlay::Nothing;
}

void setup() {
  gb.begin();
  game.state = State::initialize;
  // Activate line bellow to remove old progresson
  // gb.save.del(0);
}

void init_level() {
  // initialisation du niveau
  Joueur.En_mouvement = STOP;
  Joueur.Direction = BAS;
  Joueur.Anime = 0;
  Joueur.Pousse  = 0;
  currentLevelData = levels[game.NIVEAU_COURRANT] + 2;
  NB_LIGNES_NIVEAUX = levels[game.NIVEAU_COURRANT][1];
  NB_COLONNES_NIVEAUX = levels[game.NIVEAU_COURRANT][0];
  for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
    for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
      saveLevel[ligne * NB_COLONNES_NIVEAUX + colonne] = currentLevelData[ligne * NB_COLONNES_NIVEAUX + colonne];
    }
  }
  if ((NB_LIGNES_NIVEAUX > 16) || (NB_COLONNES_NIVEAUX > 20)) {
    pas = 3;
  } else if ((NB_LIGNES_NIVEAUX < 12) && (NB_COLONNES_NIVEAUX < 16)) {
      pas = 5;
  } else pas = 4;
  count_Boxes();
  trouve_position_perso();
  game.state = State::running; 
}

void loop() {
  gb.waitForUpdate();
  gb.display.clear();

  switch (game.state) {


    case State::initialize:
      game.NIVEAU_COURRANT = 0;
      // game.NIVEAU_COURRANT = gb.save.get(0);
      game.state = State::startmenu;
      break;


//startmenu -> running / help

    case State::startmenu:
      gb.display.drawImage(0, 0, Title);
      gb.display.setColor(WHITE);
      // gb.display.setCursor(10, 5);
      // gb.display.print("Sokoban");
      gb.display.setCursor(0, 35);
      gb.display.print("<A> Launch gamne");
      gb.display.setCursor(0, 50);
      gb.display.print("<B> Read help");

      if (gb.buttons.pressed(BUTTON_A)) {
          init_level();
      }
      if (gb.buttons.pressed(BUTTON_B)) {
        game.state = State::help;
        game.HELP_PAGE = 1;
      }
      break;


// reinit => running

    case State::reinit:
      gb.display.setColor(WHITE);
      gb.display.setCursor(0, 5);
      gb.display.print("Restart level ?");
      gb.display.setCursor(0, 25);
      gb.display.print("<A> Yes, restart");
      gb.display.setCursor(0, 40);
      gb.display.print("<B> Return to game");

      if (gb.buttons.pressed(BUTTON_A)) {
          for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
            for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
              currentLevelData[ligne * NB_COLONNES_NIVEAUX + colonne] = saveLevel[ligne * NB_COLONNES_NIVEAUX + colonne];
            }
          }
          init_level();
      }
      if (gb.buttons.pressed(BUTTON_B)) game.state = State::running;
      break;


// help -> help / running

    case State::help:
      switch (game.HELP_PAGE) {
        case 1:
          Help_screen1();
          break;
        case 2:
          Help_screen2();
          break;
        case 3:
          Help_screen3();
          break;          
      }
    break;


// map -> displaylegend / running

    case State::map:
      if (pas == 4) {
        decalageH = (20 - NB_COLONNES_NIVEAUX)*2;
        decalageV = (16 - NB_LIGNES_NIVEAUX)*2;
      } else if (pas == 3) {
          decalageH = (26 - NB_COLONNES_NIVEAUX)*1.5;
          decalageV = (20 - NB_LIGNES_NIVEAUX)*1.5;
      } else if (pas == 5) {
          decalageH = (16 - NB_COLONNES_NIVEAUX)*2.5;
          decalageV = (12 - NB_LIGNES_NIVEAUX)*2.5;        
      }
      // gb.display.printf(0, 0, "%01u", pas);
      for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
        for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
            switch (currentLevelData[(ligne)*NB_COLONNES_NIVEAUX + colonne ]) {
              case ' ':
                 gb.display.setColor(BLACK);
              break;
              case '#':
                  gb.display.setColor(RED);
              break;
              case '$':
                  gb.display.setColor(YELLOW);
              break;
              case '*':
                  gb.display.setColor(GREEN);
              break;
              case '@':
                  gb.display.setColor(WHITE);
              break;
              case '+':    
                  gb.display.setColor(BLUE);   
              break;    
              case '.':
                  gb.display.setColor(BLUE);
            } 
            gb.display.fillRect(decalageH + colonne * pas, decalageV + ligne * pas, pas, pas);
            if(currentLevelData[(ligne)*NB_COLONNES_NIVEAUX + colonne ]== '+') {
              gb.display.setColor(WHITE);
              gb.display.fillTriangle( decalageH + colonne * pas, decalageV + ligne * pas,decalageH + colonne * pas, decalageV + (ligne+1) * pas - 2,decalageH + (colonne + 1) * pas - 1, decalageV + ligne * pas);
            }
        }
      }
      gb.display.setColor(WHITE);
      gb.display.printf(8, 0, "Map of level %03u", game.NIVEAU_COURRANT + 1);
      gb.display.printf(0, 54, "<A> Legend  <B> Game");
      if (gb.buttons.pressed(BUTTON_A)) game.state = State::displaylegend;
      if (gb.buttons.pressed(BUTTON_B)) game.state = State::running;
      break;

// displaylegend -> map
      case State::displaylegend: 
        gb.display.setColor(WHITE);
        gb.display.drawRect(5, 1, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 0, "Empty space");
        gb.display.setColor(RED);
        gb.display.fillRect(5, 9, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 8, "Wall");
        gb.display.setColor(YELLOW);
        gb.display.fillRect(5, 18, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 16, "Case");
        gb.display.setColor(GREEN);
        gb.display.fillRect(5, 26, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 24, "Case on mark");
        gb.display.setColor(WHITE);
        gb.display.fillRect(5, 34, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 32, "Player");
        gb.display.setColor(BLUE);
        gb.display.fillRect(5, 42, 4, 4);
        gb.display.setColor(WHITE);
        gb.display.printf(12, 40, "Empty mark");        
        gb.display.setColor(WHITE);
        gb.display.printf(0, 54, "<B> Return to map");
        if (gb.buttons.pressed(BUTTON_B)) game.state = State::map;
        break;


// running -> map / endlevel / reinit

    case State::running:

      gb.display.setColor(WHITE);
      gb.display.printf(0, 56, "<A> Map  <B> Restart");
      gb.display.setColor(GRAY);
      gb.display.printf(60, 0, "LEVEL");
      gb.display.setColor(WHITE);
      gb.display.printf(64, 10, "%03u", game.NIVEAU_COURRANT + 1);
      // On actualise l'état du joueur
      MAJ_Joueur();

      // On actualise la position de la zone du tableau à afficher
      Position_camera(Joueur.X, Joueur.Y);

      // On affiche le contenu de chacune des cases
      DessineNiveau();
      DessinePerso();
      playSound();

      // On compte les caisses bien positionnées
      count_Boxes_On_Good_Place();

      if (PlacedBoxes == Boxes) game.state = State::endlevel;
      if (gb.buttons.pressed(BUTTON_A)) game.state = State::map;
      if (gb.buttons.pressed(BUTTON_B)) game.state = State::reinit;
      break;


// endlevel -> running / gameover

      case State::endlevel:
      if (game.NIVEAU_COURRANT < NB_NIVEAUX) {
        ++game.NIVEAU_COURRANT;
        soundtoplay = SoundToPlay::EndLevel;
        playSound();
        PlacedBoxes = 0;
        gb.display.setColor(WHITE);
        gb.display.printf(8, 0, "Launch level %03u", game.NIVEAU_COURRANT + 1);
        delay(2000);
        init_level();
        gb.save.set(0, game.NIVEAU_COURRANT);
        game.state = State::running;
      } else {
        game.state = State::gameover;
      }
      break;      
  }

}
