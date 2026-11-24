#ifndef PLAYER_H
#define PLAYER_H

#define STOP   0
#define BAS    1
#define HAUT   2
#define GAUCHE 3
#define DROITE 4

#define VITESSE_ANIME 2   // Low to go quickest - High to go slowly

struct Type_Joueur {
  int X; // Position horizontale du personnage
  int Y; // Position verticale du personnage
  uint8_t En_mouvement;
  uint8_t Direction;
  uint8_t Anime;
  uint8_t Pousse;
};

struct Type_Caisse {
  int X; // Position horizontale relative au personnage de la caisse en déplacement
  int Y; // Position verticale relative au personnage de la caisse en déplacement
};

extern Type_Joueur Joueur;
extern Type_Caisse BougeCaisse;

extern void trouve_position_perso();
extern void MAJ_Joueur();

#endif
