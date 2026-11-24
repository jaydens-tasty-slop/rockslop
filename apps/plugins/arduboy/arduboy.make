#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Arduboy compatibility layer (lib/) plus one .rock per game. Each game is a
# directory with a SOURCES-style file list below; the layer is linked into
# every rock. C++ is compiled freestanding (no exceptions, RTTI or libstdc++),
# as in celeste.make.

ARDUBOY_SRCDIR := $(APPSDIR)/plugins/arduboy
ARDUBOY_BUILDDIR := $(BUILDDIR)/apps/plugins/arduboy

ARDUBOY_LIB_C := $(addprefix $(ARDUBOY_SRCDIR)/lib/, ab_glue.c ab_libc.c)
ARDUBOY_LIB_CPP := $(addprefix $(ARDUBOY_SRCDIR)/lib/, Arduboy2.cpp \
	Arduboy2Core.cpp Arduboy2Audio.cpp Arduboy2Data.cpp Sprites.cpp \
	SpritesB.cpp ArduboyTones.cpp Print.cpp Tinyfont.cpp ab_cxx.cpp ArduboyFX.cpp)
ARDUBOY_LIB_OBJ := $(call c2obj, $(ARDUBOY_LIB_C) $(ARDUBOY_LIB_CPP))

# Games: name -> C++ sources (relative to arduboy/<name>/)
ARDUBOY_catacombs_SRC := Arduboy3D.cpp Draw.cpp Enemy.cpp Entity.cpp \
	FixedMath.cpp Font.cpp Game.cpp Map.cpp MapGenerator.cpp Menu.cpp \
	Particle.cpp Player.cpp Projectile.cpp Sounds.cpp catacombs_rb.cpp

ARDUBOY_ardugolf_SRC := camera.cpp div.cpp draw.cpp font.cpp game.cpp \
	graphics.cpp mat.cpp mul.cpp physics.cpp render_scene.cpp save.cpp \
	sincos.cpp ardugolf_rb.cpp

ARDUBOY_ardulem_SRC := Arduboy_audio.cpp Arduboy_core.cpp Ardulem.cpp \
	CustomArduboy.cpp HUD.cpp Input.cpp LEDManager.cpp Lem.cpp \
	LemManager.cpp MainMenu.cpp MapData.cpp MapManager.cpp MemoryPool.cpp \
	Music.cpp SpriteData.cpp ardulem_rb.cpp

ARDUBOY_ardrivin_SRC := main.cpp ArduboyRem.cpp ArduboyCoreRem.cpp \
	ArduboyAudioRem.cpp ardrivin_rb.cpp

ARDUBOY_shroom_SRC := shroomKnight.cpp shroom_rb.cpp

ARDUBOY_kong2_SRC := Kong2.cpp kong2_rb.cpp src/utils/Arduboy2Ext.cpp \
	src/utils/EEPROM_Utils.cpp src/entities/Bird.cpp src/entities/Birds.cpp \
	src/entities/Clapper.cpp src/entities/Clappers.cpp src/entities/Key.cpp \
	src/entities/Kong.cpp src/entities/Player.cpp src/entities/Spark.cpp \
	src/entities/Sparks.cpp

ARDUBOY_kong_SRC := Kong.cpp kong_rb.cpp src/Game.cpp \
	$(addprefix src/entities/,Barrel.cpp Base.cpp Crane.cpp Dinner.cpp \
	Fire.cpp Girder.cpp Gorilla.cpp Hook.cpp Lever.cpp Plate.cpp Player.cpp) \
	$(addprefix src/states/,BaseState.cpp HighScoreState.cpp \
	PlayGameState_activate.cpp PlayGameState_render.cpp \
	PlayGameState_update.cpp PlayGameState_utils.cpp \
	SplashScreenState.cpp TitleScreenState.cpp) \
	$(addprefix src/utils/,Arduboy2Ext.cpp EEPROM_Utils.cpp GameContext.cpp)

ARDUBOY_hellocmd_SRC := HelloCommander.cpp bmp.cpp enemy.cpp globals.cpp \
	person.cpp player.cpp stateBossTalk.cpp stateElfTurn.cpp \
	stateGameStart.cpp statePartyManagement.cpp statePlayerTurn.cpp \
	world.cpp hellocmd_rb.cpp

ARDUBOY_darkunder_SRC := DarkUnder.cpp darkunder_rb.cpp \
	$(addprefix src/controllers/,BaseController.cpp EnemyController.cpp \
	PlayerController.cpp) $(addprefix src/entities/,Base.cpp Enemy.cpp \
	Item.cpp Player.cpp) src/fonts/Font3x5.cpp src/levels/Level.cpp \
	src/utils/Arduboy2Ext.cpp src/utils/EnemyNames.cpp

ARDUBOY_castleboy_SRC := CastleBoy.cpp entity.cpp game.cpp global.cpp \
	map.cpp menu.cpp player.cpp castleboy_rb.cpp

ARDUBOY_choplifter_SRC := Choplifter.cpp choplifter_rb.cpp \
	src/utils/Arduboy2Ext.cpp

ARDUBOY_mysticballoon_SRC := MYBL_AB.cpp mysticballoon_rb.cpp

ARDUBOY_samegame_SRC := MyArduboy.cpp common.cpp editor.cpp game.cpp logo.cpp \
	menu.cpp title.cpp samegame.cpp samegame_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_trollyfish_SRC := FISH_AB.cpp trollyfish_rb.cpp

ARDUBOY_blobattack_SRC := BLBA_AB.cpp blobattack_rb.cpp

ARDUBOY_shadowrunner_SRC := SHRUN_AB.cpp shadowrunner_rb.cpp

ARDUBOY_knightmove_SRC := MyArduboy.cpp common.cpp game.cpp knightmove.cpp logo.cpp menu.cpp title.cpp knightmove_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_quarto_SRC := MyArduboy.cpp common.cpp game.cpp logo.cpp menu.cpp quarto.cpp title.cpp quarto_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_psicolo_SRC := MyArduboy.cpp common.cpp game.cpp logo.cpp menu.cpp psicolo.cpp title.cpp psicolo_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_sirene_SRC := SRN_AB.cpp sirene_rb.cpp

ARDUBOY_rmh_SRC := RMH_AB.cpp rmh_rb.cpp

ARDUBOY_dicefate_SRC := DICE_AB.cpp dicefate_rb.cpp

ARDUBOY_begemmed_SRC := BGEM_AB.cpp begemmed_rb.cpp

ARDUBOY_virus_SRC := VLQP_AB.cpp bullet.cpp door.cpp elements.cpp enemies.cpp game.cpp globals.cpp level.cpp menu.cpp pickup.cpp player.cpp virus_rb.cpp

ARDUBOY_bubblepop_SRC := BPOP_AB.cpp bubblepop_rb.cpp

ARDUBOY_lasers_SRC := MyArduboy.cpp common.cpp game.cpp lasers.cpp logo.cpp menu.cpp title.cpp warn.cpp lasers_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_hollow_SRC := MyArduboy.cpp game.cpp hollow.cpp logo.cpp title.cpp hollow_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_hopper_SRC := MyArduboy.cpp game.cpp hopper.cpp logo.cpp title.cpp hopper_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_chiemagari_SRC := MyArduboy.cpp chiemagari.cpp common.cpp gallery.cpp logo.cpp menu.cpp puzzle.cpp chiemagari_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_obreversi_SRC := MyArduboy.cpp common.cpp game.cpp logo.cpp menu.cpp reversi.cpp title.cpp obreversi_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_chribocchi_SRC := MyArduboy.cpp chribocchi.cpp game.cpp logo.cpp title.cpp chribocchi_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_fantasyrampage_SRC := FRMP_AB.cpp fantasyrampage_rb.cpp ../atmlib/ATMlib.cpp

ARDUBOY_sunfire_SRC := SUNF_AB.cpp sunfire_rb.cpp ../atmlib/ATMlib.cpp

ARDUBOY_n43_SRC := Base.cpp Bullet.cpp Enemy.cpp EnemyBullet.cpp Nineteen43.cpp Obstacle.cpp Plane.cpp Player.cpp n43_rb.cpp

ARDUBOY_n44_SRC := Nineteen44.cpp src/Entities/Base.cpp src/Entities/Bullet.cpp src/Entities/Enemy.cpp src/Entities/EnemyBullet.cpp src/Entities/Obstacle.cpp src/Entities/Plane.cpp src/Entities/Player.cpp src/Utils/Arduboy2Ext.cpp n44_rb.cpp

ARDUBOY_outpostdefense_SRC := OPDE_AB.cpp outpostdefense_rb.cpp

ARDUBOY_helmetshordes_SRC := HEHO_AB.cpp helmetshordes_rb.cpp

ARDUBOY_pinball_SRC := PNBL_AB.cpp pinball_rb.cpp

ARDUBOY_ohmummy_SRC := OhMummy.cpp ohmummy_rb.cpp

ARDUBOY_rooftop_SRC := rooftop.cpp rooftop_rb.cpp

ARDUBOY_towerofpain_SRC := TowerOfPain.cpp src/Game.cpp towerofpain_rb.cpp

ARDUBOY_evasion_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp common.cpp evasion.cpp game.cpp logo.cpp menu.cpp title.cpp evasion_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_nightraid_SRC := night-raid.cpp nightraid_rb.cpp

ARDUBOY_ardubullets_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp ardubullets.cpp common.cpp game.cpp logo.cpp menu.cpp title.cpp ardubullets_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_gosencho_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp common.cpp game.cpp gosencho.cpp logo.cpp menu.cpp title.cpp gosencho_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_toyokumono_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp common.cpp game.cpp logo.cpp menu.cpp title.cpp toyokumono.cpp toyokumono_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_rysk_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp rysk.cpp rysk_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_bananonsense_SRC := MyArduboy2.cpp MyArduboyPlaytune.cpp bananonsense.cpp bindata.cpp common.cpp game.cpp logo.cpp menu.cpp title.cpp bananonsense_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_morse_SRC := Decoder.cpp Encoder.cpp FakeKeyboard.cpp Iambic.cpp MyArduboy2.cpp MyArduboyPlaytune.cpp bindata.cpp common.cpp console.cpp credit.cpp list.cpp logo.cpp menu.cpp morse.cpp setting.cpp morse_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_starflight_SRC := SFL_AB.cpp starflight_rb.cpp

ARDUBOY_teenytank_SRC := TeenyTank.cpp bitmaps.cpp teenytank_rb.cpp

ARDUBOY_undertower_SRC := undertower.cpp undertower_rb.cpp

ARDUBOY_arduman_SRC := arduman_arduboylib11.cpp ardu_main.cpp Fruit.cpp Ghost.cpp Highscores.cpp Player.cpp Utils.cpp arduman_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_galaxion_SRC := galaxion_main.cpp enemy.cpp game.cpp ship.cpp galaxion_rb.cpp ../lib/ArduboyPlaytune.cpp ../lib/ab_new.cpp

ARDUBOY_chickenx_SRC := ChickenX.cpp Collision.cpp titlebmp.cpp chickenx_rb.cpp

ARDUBOY_crateconfusion_SRC := crate-confusion.cpp common.cpp credits.cpp game.cpp title.cpp crateconfusion_rb.cpp

ARDUBOY_shatteredlands_SRC := shatteredlands.cpp shatteredlands_rb.cpp

ARDUBOY_shatteredlands2_SRC := shatteredlands2.cpp shatteredlands2_rb.cpp

ARDUBOY_evade_SRC := evade_main.cpp evade_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_ppblackjack_SRC := ppblackjack.cpp src/Game.cpp src/fonts/Font3x5.cpp src/states/BaseState.cpp src/states/GameLoseState.cpp src/states/GameWinState.cpp src/states/PlayGameState.cpp src/states/PlayGameState_Buttons.cpp src/states/PlayGameState_Play.cpp src/states/PlayGameState_Render.cpp src/states/PlayGameState_Utils.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/GameContext.cpp ppblackjack_rb.cpp

ARDUBOY_arduwars_SRC := arduwars_main.cpp AWGame.cpp DataClasses.cpp arduwars_rb.cpp ../lib/ab_new.cpp

ARDUBOY_dungeons2_SRC := DUN2_AB.cpp dungeons2_rb.cpp ../lib/ab_new.cpp

ARDUBOY_poopanic_SRC := poop-panic.cpp poopanic_rb.cpp

ARDUBOY_flappyball_SRC := FlappyBall.cpp flappyball_rb.cpp

ARDUBOY_farkle_SRC := farkle.cpp src/Game.cpp src/fonts/Font3x5.cpp src/fonts/Font4x6.cpp src/states/BaseState.cpp src/states/GameOverState.cpp src/states/HighScoreState.cpp src/states/PlayGameState.cpp src/states/PlayerNamesState.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/EEPROM_Utils.cpp src/utils/GameContext.cpp src/utils/Name.cpp farkle_rb.cpp

ARDUBOY_cribbage_SRC := cribbage.cpp src/Game.cpp src/entities/Deck.cpp src/entities/Player.cpp src/entities/Score.cpp src/fonts/Font3x5.cpp src/states/BaseState.cpp src/states/GameOverState.cpp src/states/PlayGameState_Activate.cpp src/states/PlayGameState_Render.cpp src/states/PlayGameState_Update.cpp src/states/PlayGameState_Utils.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/CardUtils.cpp src/utils/EEPROM_Utils.cpp src/utils/GameContext.cpp cribbage_rb.cpp

ARDUBOY_germanwhist_SRC := germanwhist.cpp src/Game.cpp src/entities/Deck.cpp src/entities/Player.cpp src/fonts/Font3x5.cpp src/states/BaseState.cpp src/states/GameOverState.cpp src/states/PlayGameState_Activate.cpp src/states/PlayGameState_Render.cpp src/states/PlayGameState_Update.cpp src/states/PlayGameState_Utils.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/CardUtils.cpp src/utils/GameContext.cpp germanwhist_rb.cpp

ARDUBOY_pppicross_SRC := pppicross.cpp src/Game.cpp src/entities/Puzzle.cpp src/fonts/Font3x5.cpp src/states/BaseState.cpp src/states/PlayGameState_Activate.cpp src/states/PlayGameState_Render.cpp src/states/PlayGameState_Update.cpp src/states/PlayGameState_Utils.cpp src/states/SelectPuzzleState.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/GameContext.cpp pppicross_rb.cpp

ARDUBOY_minirogue_SRC := minirogue.cpp src/Game.cpp src/fonts/Font3x5.cpp src/states/BaseState.cpp src/states/EventState.cpp src/states/FightMonstersState.cpp src/states/GameOverState.cpp src/states/MerchantState.cpp src/states/RestingState.cpp src/states/ShowCardsState.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/states/TrapState.cpp src/states/TreasureState.cpp src/utils/Arduboy2Ext.cpp src/utils/GameContext.cpp minirogue_rb.cpp

ARDUBOY_turtlebridge_SRC := src/Game.cpp src/entities/Fish.cpp src/entities/Player.cpp src/entities/Turtle.cpp src/states/BaseState.cpp src/states/HighScoreState.cpp src/states/PlayGameState.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/EEPROM_Utils.cpp src/utils/GameContext.cpp turtlebridge.cpp turtlebridge_rb.cpp

ARDUBOY_firepanic_SRC := firepanic.cpp src/Game.cpp src/entities/Ambulance.cpp src/entities/Angel.cpp src/entities/Coin.cpp src/entities/OtherCar.cpp src/entities/Player.cpp src/entities/Victim.cpp src/states/BaseState.cpp src/states/GameIntroState.cpp src/states/HighScoreState.cpp src/states/PlayGameState.cpp src/states/RaceState.cpp src/states/SplashScreenState.cpp src/states/TitleScreenState.cpp src/utils/Arduboy2Ext.cpp src/utils/EEPROM_Utils.cpp src/utils/GameContext.cpp firepanic_rb.cpp

ARDUBOY_oilpanic_SRC := oilpanic.cpp src/fonts/Font3x5.cpp src/fonts/Font4x6.cpp src/utils/Arduboy2Ext.cpp src/utils/EEPROM_Utils.cpp oilpanic_rb.cpp

ARDUBOY_logix_SRC := logix.cpp src/fonts/Font4x6.cpp src/utils/Arduboy2Ext.cpp logix_rb.cpp

ARDUBOY_ardubuggy_SRC := ArduBuggy.cpp ardubuggy_rb.cpp

ARDUBOY_pongab_SRC := pong.cpp ball.cpp game.cpp paddle.cpp vector.cpp pongab_rb.cpp

ARDUBOY_liteout_SRC := LiteOut.cpp liteout_rb.cpp

ARDUBOY_tres_SRC := tres.cpp tres_rb.cpp

ARDUBOY_trenchrun_SRC := TrenchRun.cpp trenchrun_rb.cpp

ARDUBOY_hangman_SRC := Hangman.cpp hangman_rb.cpp

ARDUBOY_dominoes_SRC := dominoes.cpp src/fonts/Font4x6.cpp src/utils/Arduboy2Ext.cpp src/utils/EEPROM_Utils.cpp src/utils/Name.cpp dominoes_rb.cpp

ARDUBOY_junofirst_SRC := junofirst.cpp src/utils/Arduboy2Ext.cpp junofirst_rb.cpp

ARDUBOY_spacecab_SRC := spacecab.cpp src/Fonts/Font4x6.cpp src/Utils/Arduboy2Ext.cpp spacecab_rb.cpp

ARDUBOY_boris_SRC := boris_goes_skiing.cpp boris_rb.cpp

ARDUBOY_lion_SRC := lion.cpp src/utils/Arduboy2Ext.cpp lion_rb.cpp

ARDUBOY_buttonstrail_SRC := buttonstrail.cpp src/fonts/Font3x5.cpp src/utils/Arduboy2Ext.cpp buttonstrail_rb.cpp

ARDUBOY_roadtrip_SRC := roadtrip.cpp src/utils/Arduboy2Ext.cpp roadtrip_rb.cpp

ARDUBOY_stellarimpact_SRC := Stellar_Impact.cpp stellarimpact_rb.cpp

ARDUBOY_arduroad_SRC := arduroad.cpp src/fonts/Font4x6.cpp src/utils/Arduboy2Ext.cpp arduroad_rb.cpp

ARDUBOY_loverush_SRC := LoveRush.cpp loverush_rb.cpp

ARDUBOY_helii_SRC := Helii-Arduboy.cpp CaveManager.cpp Column.cpp Drawing.cpp Player.cpp SmokeTrail.cpp helii_rb.cpp

ARDUBOY_ardulo_SRC := ArduLO.cpp Common.cpp GameEngine.cpp GameScene.cpp TitleScene.cpp ardulo_rb.cpp

ARDUBOY_layingpipe_SRC := layingpipe.cpp layingpipe_rb.cpp

ARDUBOY_glove_SRC := badguy.cpp bullet.cpp exit.cpp explorer.cpp game.cpp globals.cpp glove.cpp key.cpp mainmenu.cpp options.cpp physics.cpp prompt.cpp records.cpp save.cpp spawner.cpp title.cpp treasure.cpp wall.cpp world.cpp glove_rb.cpp

ARDUBOY_omegachase_SRC := Omega_chase_V_1_0.cpp omegachase_rb.cpp

ARDUBOY_humanityrevenge_SRC := Humanity_Revenge_DC.cpp humanityrevenge_rb.cpp

ARDUBOY_cosmicpods_SRC := CosmicPods.cpp cosmicpods_rb.cpp

ARDUBOY_cyberhack_SRC := cyberhack.cpp src/fonts/Font3x5.cpp src/utils/Arduboy2Ext.cpp cyberhack_rb.cpp

ARDUBOY_randocity_SRC := Randocity.cpp FatFont.cpp Records.cpp Timer.cpp World.cpp lfsr.cpp randocity_rb.cpp

ARDUBOY_leword_SRC := leword.cpp src/utils/Arduboy2Ext.cpp leword_rb.cpp

ARDUBOY_beamemup_SRC := BeamEmUp.cpp beamemup_rb.cpp

ARDUBOY_ravine_SRC := RavineDespoiler.cpp ravine_rb.cpp

ARDUBOY_ponghauki_SRC := PongHauKi.cpp src/fonts/Font4x6.cpp ponghauki_rb.cpp

ARDUBOY_trialsofastarok_SRC := AstarokGame_Logic.cpp AstarokGame_Render.cpp src/entities/AISprite.cpp src/entities/InteractiveObject.cpp src/entities/Map.cpp src/entities/Room.cpp src/entities/Sprite.cpp src/utils/Arduboy2Ext.cpp trialsofastarok.cpp trialsofastarok_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_ardecipher_SRC := ARDecipher.cpp src/fonts/Font4x6.cpp ardecipher_rb.cpp

ARDUBOY_crates3d_SRC := Crates3D.cpp crates3d_rb.cpp

ARDUBOY_floodfill_SRC := arduboy_floodfill.cpp floodfill_rb.cpp

ARDUBOY_snakecdr_SRC := SNAKE.cpp snakecdr_rb.cpp

ARDUBOY_backtojungle_SRC := backtojungle.cpp backtojungle_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_ringpuzzle_SRC := Assets.cpp Background.cpp CompletePuzzleScreen.cpp CreditsScreen.cpp FlashingMessage.cpp Game.cpp GameOverScreen.cpp GameStage.cpp MainScreen.cpp Score.cpp Stage.cpp StageBase.cpp TimePrinter.cpp TransitionStage.cpp VStripesTransition.cpp WaitStage.cpp ringpuzzle.cpp ringpuzzle_rb.cpp

ARDUBOY_sfcave_SRC := SFCave.cpp sfcave_rb.cpp

ARDUBOY_waternetab_SRC := src/commonvars.cpp src/cursor.cpp src/game.cpp src/helperfuncs.cpp src/helpscreens.cpp src/intro.cpp src/level.cpp src/levelscleared.cpp src/levelselect.cpp src/printfuncs.cpp src/savestate.cpp src/sound.cpp src/titlescreen.cpp waternetab.cpp waternetab_rb.cpp

ARDUBOY_apara_SRC := APara.cpp apara_rb.cpp

ARDUBOY_diamonds_SRC := diamonds.cpp game.cpp diamonds_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_asteroidsx_SRC := STASTEROIDS.cpp asteroidsx_rb.cpp

ARDUBOY_ardusweeper_SRC := minesweeper.cpp ardusweeper_rb.cpp

ARDUBOY_blockspush_SRC := blocks.cpp levels.cpp blockspush_rb.cpp

ARDUBOY_blockstacker_SRC := blockstacker.cpp blockstacker_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_quadrastic_SRC := Quadrastic.cpp quadrastic_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_keykatit_SRC := keykat-it.cpp keykatit_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_microtank_SRC := MicroTank.cpp microtank_rb.cpp

ARDUBOY_starhonor_SRC := GUI.cpp Globals.cpp Map.cpp SelectionArrow.cpp Ship.cpp StarField.cpp Text.cpp Vector2d.cpp starhonor.cpp starhonor_rb.cpp ../lib/ArduboyPlaytune.cpp ../lib/ab_new.cpp

ARDUBOY_joustish_SRC := joustish.cpp game.cpp draw.cpp highscores.cpp joustish_rb.cpp

ARDUBOY_spacewads_SRC := Space-Wads.cpp spacewads_rb.cpp

ARDUBOY_littlebeluga_SRC := littlebeluga.cpp littlebeluga_rb.cpp

ARDUBOY_spacefighter_SRC := SpaceFigther_bitmaps.cpp collision.cpp draw.cpp generator.cpp movement.cpp spacefighter.cpp spacefighter_rb.cpp

ARDUBOY_jumpingjack_SRC := jumping_jack.cpp jumpingjack_rb.cpp

ARDUBOY_streetracer_SRC := StreetRacer.cpp streetracer_rb.cpp

ARDUBOY_newblocks_SRC := NewBlocksOnTheKid.cpp newblocks_rb.cpp

ARDUBOY_tunnelfighter_SRC := tunnelFighter.cpp tunnelfighter_rb.cpp

ARDUBOY_tilesfv_SRC := Tiles.cpp tilesfv_rb.cpp

ARDUBOY_gohome_SRC := GoHome.cpp gohome_rb.cpp

ARDUBOY_montana_SRC := Montana.cpp montana_rb.cpp

ARDUBOY_calculate_SRC := Calculate.cpp calculate_rb.cpp

ARDUBOY_slidexl_SRC := Slide-XL.cpp slidexl_rb.cpp

ARDUBOY_tinydash_SRC := Tiny-Dash.cpp tinydash_rb.cpp

ARDUBOY_arduskiing_SRC := ArduSkiing.cpp arduskiing_rb.cpp

ARDUBOY_lasermaze_SRC := LaserMaze.cpp lasermaze_rb.cpp

ARDUBOY_arduel_SRC := arduel.cpp arduel_rb.cpp

ARDUBOY_mouseescape_SRC := MouseEscape.cpp Food.cpp Level.cpp Maus.cpp Snake.cpp mouseescape_rb.cpp ../lib/ab_new.cpp

ARDUBOY_militaryaviator_SRC := MilitaryAviator.cpp militaryaviator_rb.cpp

ARDUBOY_candlelight_SRC := Candlelight.cpp candlelight_rb.cpp

ARDUBOY_bladerunner_SRC := BladeRunner.cpp bladerunner_rb.cpp

ARDUBOY_hunt_SRC := game.cpp hunt_rb.cpp ../lib/ab_new.cpp

ARDUBOY_minotaur_SRC := src.cpp sprites.cpp minotaur_rb.cpp

ARDUBOY_batgame_SRC := batandcollisions.cpp batgame_rb.cpp

ARDUBOY_calldungeon_SRC := dungeon.cpp game.cpp player.cpp calldungeon_rb.cpp ../lib/ab_new.cpp

ARDUBOY_breakouttv_SRC := main.cpp breakouttv_rb.cpp

ARDUBOY_breakouthiro_SRC := sketch_jun20a.cpp breakouthiro_rb.cpp

ARDUBOY_tunnelterror_SRC := Tunnel_Terror.cpp tunnelterror_rb.cpp

ARDUBOY_tiny2048_SRC := ino.cpp game.cpp anime.cpp panel.cpp score.cpp sprite.cpp lib/common.cpp lib/eep.cpp lib/frame.cpp lib/key.cpp lib/math.cpp lib/oled.cpp lib/rnd.cpp lib/snd.cpp lib/sys.cpp lib/tinymt32.c tiny2048_rb.cpp

ARDUBOY_tinyasteroids_SRC := ino.cpp game.cpp ast.cpp beep.cpp bullet.cpp chr.cpp res.cpp score.cpp ufo.cpp lib/common.cpp lib/eep.cpp lib/frame.cpp lib/key.cpp lib/math.cpp lib/oled.cpp lib/rnd.cpp lib/snd.cpp lib/sys.cpp lib/tinymt32.c tinyasteroids_rb.cpp

ARDUBOY_tinynstower_SRC := ino.cpp game.cpp beep.cpp chr.cpp pos.cpp res.cpp score.cpp stage.cpp lib/common.cpp lib/eep.cpp lib/frame.cpp lib/key.cpp lib/math.cpp lib/oled.cpp lib/rnd.cpp lib/snd.cpp lib/sys.cpp lib/tinymt32.c tinynstower_rb.cpp

ARDUBOY_tinyinvader_SRC := ino.cpp game.cpp chr.cpp enemy.cpp mem.cpp res.cpp lib/common.cpp lib/eep.cpp lib/frame.cpp lib/key.cpp lib/math.cpp lib/oled.cpp lib/rnd.cpp lib/snd.cpp lib/sys.cpp lib/tinymt32.c tinyinvader_rb.cpp

ARDUBOY_tinydepth_SRC := ino.cpp game.cpp beep.cpp bullet.cpp enemy.cpp menu.cpp score.cpp ship.cpp lib/common.cpp lib/eep.cpp lib/frame.cpp lib/key.cpp lib/math.cpp lib/oled.cpp lib/snd.cpp lib/sys.cpp lib/tinymt32.c tinydepth_rb.cpp

ARDUBOY_easterpanic_SRC := EasterPanic.cpp Game.cpp GameplayState.cpp LevelSelectState.cpp SplashScreenState.cpp TitleScreenState.cpp easterpanic_rb.cpp ../lib/ab_new.cpp

ARDUBOY_ardubloxx_SRC := ArduBloxx.cpp ardubloxx_rb.cpp

ARDUBOY_magic8ball_SRC := ArduboyMagic8Ball.cpp magic8ball_rb.cpp

ARDUBOY_ardusnake_SRC := ArduSnake.cpp ardusnake_rb.cpp

ARDUBOY_unicorndash_SRC := unicorn_dash.cpp unicorndash_rb.cpp

ARDUBOY_hippodrome_SRC := hippodrome.cpp hippodrome_rb.cpp

ARDUBOY_ardunback_SRC := ardu-n-back.cpp ardunback_rb.cpp

ARDUBOY_tronduboy_SRC := tron.cpp tronduboy_rb.cpp

ARDUBOY_shooter_SRC := shooter.cpp shooter_rb.cpp

ARDUBOY_ardutan_SRC := ardutan.cpp Ball.cpp BallManager.cpp Block.cpp BlockManager.cpp Button.cpp GameObject.cpp ardutan_rb.cpp ../lib/ab_new.cpp

ARDUBOY_quadfall_SRC := Quadfall.cpp game.cpp input.cpp renderer.cpp sound.cpp quadfall_rb.cpp

ARDUBOY_late_SRC := LATE.cpp common.cpp game_1010.cpp game_bb.cpp game_tetris.cpp late_rb.cpp

ARDUBOY_technomonkey_SRC := TechnoMonkey.cpp Bear.cpp Elephant.cpp Hippo.cpp Lion.cpp Player.cpp Vulture.cpp Zookeeper.cpp technomonkey_rb.cpp

ARDUBOY_flappyhalloween_SRC := flappyhalloween.cpp flight.cpp bitmaps.cpp flappyhalloween_rb.cpp

ARDUBOY_zuthsrah_SRC := Zuthsrah.cpp Character.cpp Enemies.cpp Starz.cpp Weapons.cpp bitmaps.cpp physics.cpp zuthsrah_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_rowfarm_SRC := RowFarm.cpp rowfarm_rb.cpp

ARDUBOY_snack_SRC := snack.cpp snack_rb.cpp

ARDUBOY_spaceshooter_SRC := space.cpp bitmaps.cpp spaceshooter_rb.cpp

ARDUBOY_tamaguino_SRC := Tamaguino-Arduboy.cpp tamaguino_rb.cpp

ARDUBOY_jumpboy_SRC := jumpboy.cpp jumpboy_rb.cpp

ARDUBOY_jumpboy2_SRC := jumpboy2.cpp jumpboy2_rb.cpp

ARDUBOY_boneshakers_SRC := BoneShakers.cpp Draw.cpp FixedMath.cpp Font.cpp Game.cpp boneshakers_rb.cpp

ARDUBOY_ecomd_SRC := ECOMD_AB.cpp ecomd_rb.cpp

ARDUBOY_picoracers_SRC := PIRC_AB.cpp picoracers_rb.cpp

ARDUBOY_coinrush_SRC := Arduboy_Game_1.cpp coinrush_rb.cpp

ARDUBOY_citylife_SRC := arduboy_City_Life.cpp Font4x6.cpp citylife_rb.cpp

ARDUBOY_cooties_SRC := cooties.cpp Font3x5.cpp cooties_rb.cpp

ARDUBOY_microflight_SRC := FlightSim.cpp FixedMath.cpp Font.cpp Game.cpp microflight_rb.cpp

ARDUBOY_ddrboy_SRC := ddrboy.cpp game.cpp ddrboy_rb.cpp

ARDUBOY_arduboy1010_SRC := arduboy-1010.cpp arduboy1010_rb.cpp

ARDUBOY_arenafight_SRC := Globals.cpp actor.cpp bossActors.cpp enemy.cpp fight.cpp fightEngine.cpp gameEngine.cpp player.cpp  arenafight_rb.cpp

ARDUBOY_amazeab_SRC := amazeab.cpp amazeab_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_anykindcar_SRC := anykindcar.cpp anykindcar_rb.cpp

ARDUBOY_ultrahop_SRC := ultrahop.cpp ultrahop_rb.cpp

ARDUBOY_athleticboy_SRC := athleticboy.cpp athleticboy_rb.cpp

ARDUBOY_eggmaster_SRC := eggmaster.cpp eggmaster_rb.cpp

ARDUBOY_socialdist_SRC := socialdist.cpp pedestrian.cpp player.cpp socialdist_rb.cpp

ARDUBOY_bounceab_SRC := bounceab.cpp Maps.cpp Player.cpp Sprites.cpp bounceab_rb.cpp

ARDUBOY_nielk_SRC := nielk.cpp nielk_rb.cpp

ARDUBOY_arduboydash_SRC := arduboydash.cpp arduboydash_rb.cpp

ARDUBOY_guessdots_SRC := guessdots.cpp guessdots_rb.cpp

ARDUBOY_trianglewave_SRC := trianglewave.cpp trianglewave_rb.cpp

ARDUBOY_ardugo_SRC := ardugo_main.cpp ArduoGo.cpp Board.cpp move.cpp sidebar.cpp simple-play.cpp simple-rules.cpp validate.cpp ardugo_rb.cpp

ARDUBOY_particles_SRC := particles.cpp particles_rb.cpp

ARDUBOY_wontsurvive_SRC := wontsurvive.cpp wontsurvive_rb.cpp

ARDUBOY_poitto_SRC := AChr.cpp ArrowChr.cpp BulletChr.cpp Chr.cpp Clear.cpp EChr.cpp Game.cpp GoalChr.cpp HalfChr.cpp HideChr.cpp MapChr.cpp MyChr.cpp Over.cpp Ready.cpp SpringChr.cpp SwitchChr.cpp Title.cpp poitto_main.cpp poitto_rb.cpp ../lib/ab_new.cpp

ARDUBOY_microtdab_SRC := microtdab.cpp microtdab_rb.cpp

ARDUBOY_submarine_SRC := i4arduboy.cpp actor.cpp modules.cpp util.cpp submarine_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_quarth_SRC := DashBoard.cpp GameController.cpp Highscore.cpp SpaceShip.cpp StarField.cpp quarth_main.cpp quarth_rb.cpp quarth_rb.cpp ../lib/ab_new.cpp

ARDUBOY_armageddon_SRC := armageddon.cpp armageddon_rb.cpp

ARDUBOY_bangbang_SRC := bangbang.cpp bullet.cpp cannon.cpp terrain.cpp bangbang_rb.cpp

ARDUBOY_jezzball_SRC := jezzball.cpp ball.cpp board.cpp cursor.cpp line.cpp jezzball_rb.cpp ../lib/ab_new.cpp

ARDUBOY_solitairegb_SRC := solitairegb.cpp card.cpp pile.cpp undo.cpp solitairegb_rb.cpp ../lib/ab_new.cpp

ARDUBOY_supercratebuino_SRC := supercratebuino.cpp supercratebuino_rb.cpp

ARDUBOY_questtruth_SRC := questtruth.cpp questtruth_rb.cpp src/Game.cpp src/BoxView.cpp src/DialogManager.cpp src/EntitiesManager.cpp src/ItemsManager.cpp src/Menu.cpp src/SoundManager.cpp src/drawable/Character.cpp src/drawable/Enemy.cpp src/drawable/Item.cpp src/drawable/Map.cpp src/drawable/Player.cpp

ARDUBOY_arduwhack_SRC := arduwhack.cpp arduwhack_rb.cpp

ARDUBOY_microcityab_SRC := microcityab.cpp microcityab_rb.cpp Building.cpp Connectivity.cpp Draw.cpp Font.cpp Game.cpp Interface.cpp Simulation.cpp Strings.cpp Terrain.cpp

ARDUBOY_tinydungeon_SRC := tinydungeon.cpp tinydungeon_rb.cpp dungeon.cpp bitmapDrawing.cpp soundFX.cpp TinyJoypadUtils.cpp

ARDUBOY_cursedmansion_SRC := cursedmansion.cpp cursedmansion_rb.cpp

ARDUBOY_othelloab_SRC := othelloab.cpp othelloab_rb.cpp data.cpp logic.cpp visual.cpp

ARDUBOY_rundino_SRC := rundino.cpp rundino_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_rundhalloween_SRC := rundhalloween.cpp rundhalloween_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_defenduino_SRC := Baiter.cpp Bomber.cpp Enemy.cpp EnemyShot.cpp Font3x5.cpp GameCamera.cpp GameObject.cpp GameOverState.cpp GameState.cpp Humanoid.cpp Lander.cpp Landscape.cpp MenuState.cpp Mine.cpp MovingGameObject.cpp Particles.cpp PlayerShip.cpp PlayerShot.cpp Pod.cpp Renderer.cpp Swarmer.cpp Vector2Byte.cpp Vector2Int.cpp defenduino.cpp defenduino_rb.cpp ../lib/ab_new.cpp

ARDUBOY_pyoro_SRC := pyoro.cpp pyoro_rb.cpp

ARDUBOY_harambe_SRC := harambe.cpp harambe_rb.cpp ../lib/ab_new.cpp

ARDUBOY_kungfuescape_SRC := kungfuescape.cpp kungfuescape_rb.cpp

ARDUBOY_roshambo_SRC := roshambo.cpp roshambo_rb.cpp

ARDUBOY_xmasgame_SRC := xmasgame.cpp xmasgame_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_creapawn_SRC := creapawn.cpp creapawn_rb.cpp ../lib/ab_new.cpp

ARDUBOY_cannonfather_SRC := cannonfather.cpp cannonfather_rb.cpp

ARDUBOY_escapefromearth_SRC := escapefromearth.cpp escapefromearth_rb.cpp

ARDUBOY_pocketrocket_SRC := pocketrocket.cpp pocketrocket_rb.cpp

ARDUBOY_minigolf_SRC := minigolf.cpp src/Font4x6/Font4x6.cpp minigolf_rb.cpp

ARDUBOY_bibleblaster_SRC := bibleblaster.cpp bibleblaster_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_tiretester_SRC := tiretester.cpp tiretester_rb.cpp

ARDUBOY_thefall_SRC := thefall.cpp thefall_rb.cpp

ARDUBOY_rhombus_SRC := rhombus.cpp rhombus_rb.cpp

ARDUBOY_kube_SRC := kube.cpp kube_rb.cpp

ARDUBOY_mastermind_SRC := mastermind.cpp mastermind_rb.cpp

ARDUBOY_pegsolitaire_SRC := pegsolitaire.cpp pegsolitaire_rb.cpp

ARDUBOY_pi24k_SRC := MyArduboy.cpp common.cpp logo.cpp main.cpp pi.cpp pi24k.cpp pi24k_rb.cpp ../lib/ArduboyPlaytune.cpp

ARDUBOY_firebox_SRC := firebox.cpp firebox_rb.cpp

ARDUBOY_frogger_SRC := frogger.cpp frogger_rb.cpp Font3x5.cpp

ARDUBOY_bangi_SRC := bangi.cpp Customlib.cpp bangi_rb.cpp

ARDUBOY_fidgetspinner_SRC := fidgetspinner.cpp fidgetspinner_rb.cpp

ARDUBOY_GAMES := catacombs ardugolf ardulem ardrivin shroom kong2 kong hellocmd darkunder castleboy choplifter mysticballoon samegame trollyfish blobattack shadowrunner knightmove quarto psicolo sirene rmh dicefate begemmed virus bubblepop lasers hollow hopper chiemagari obreversi chribocchi fantasyrampage sunfire n43 n44 outpostdefense helmetshordes pinball ohmummy rooftop towerofpain evasion nightraid ardubullets gosencho toyokumono rysk bananonsense morse starflight teenytank undertower arduman galaxion chickenx crateconfusion shatteredlands shatteredlands2 evade ppblackjack arduwars dungeons2 poopanic flappyball farkle cribbage germanwhist pppicross minirogue turtlebridge firepanic oilpanic logix ardubuggy pongab liteout tres trenchrun hangman dominoes junofirst spacecab boris lion buttonstrail roadtrip stellarimpact arduroad loverush helii ardulo layingpipe glove omegachase humanityrevenge cosmicpods cyberhack randocity leword beamemup ravine ponghauki trialsofastarok ardecipher crates3d floodfill snakecdr backtojungle ringpuzzle sfcave waternetab apara diamonds asteroidsx ardusweeper blockspush blockstacker quadrastic keykatit microtank starhonor joustish spacewads littlebeluga spacefighter jumpingjack streetracer newblocks tunnelfighter tilesfv gohome montana calculate slidexl tinydash arduskiing lasermaze arduel mouseescape militaryaviator candlelight bladerunner hunt minotaur batgame calldungeon breakouttv breakouthiro tunnelterror tiny2048 tinyasteroids tinynstower tinyinvader tinydepth easterpanic ardubloxx magic8ball ardusnake unicorndash hippodrome ardunback tronduboy shooter ardutan quadfall late technomonkey flappyhalloween zuthsrah rowfarm snack spaceshooter tamaguino jumpboy jumpboy2 boneshakers ecomd picoracers coinrush citylife cooties microflight ddrboy arduboy1010 arenafight amazeab anykindcar ultrahop athleticboy eggmaster socialdist bounceab nielk arduboydash guessdots trianglewave ardugo particles wontsurvive poitto microtdab submarine quarth armageddon bangbang jezzball solitairegb supercratebuino questtruth arduwhack microcityab tinydungeon cursedmansion othelloab rundino rundhalloween defenduino pyoro harambe kungfuescape roshambo xmasgame creapawn cannonfather escapefromearth pocketrocket minigolf bibleblaster tiretester thefall rhombus kube mastermind pegsolitaire pi24k firebox frogger bangi fidgetspinner

ARDUBOY_ALL_CPP := $(ARDUBOY_LIB_CPP) $(foreach g,$(ARDUBOY_GAMES),\
	$(addprefix $(ARDUBOY_SRCDIR)/$(g)/,$(ARDUBOY_$(g)_SRC)))
ARDUBOY_HEADERS := $(wildcard $(ARDUBOY_SRCDIR)/lib/*.h \
	$(ARDUBOY_SRCDIR)/lib/avr/*.h $(ARDUBOY_SRCDIR)/*/*.h)

OTHER_SRC += $(ARDUBOY_LIB_C)
ARDUBOY_ROCKS := $(foreach g,$(ARDUBOY_GAMES),$(ARDUBOY_BUILDDIR)/$(g).rock)
ROCKS += $(ARDUBOY_ROCKS)

define ARDUBOY_GAME_RULE
ARDUBOY_$(1)_OBJ := $$(call c2obj, $$(addprefix $$(ARDUBOY_SRCDIR)/$(1)/,$$(ARDUBOY_$(1)_SRC)))
$$(ARDUBOY_BUILDDIR)/$(1).rock: $$(ARDUBOY_$(1)_OBJ) $$(ARDUBOY_LIB_OBJ)
	$$(call PRINTS,LD $$(@F))$$(CC) $$(PLUGINFLAGS) -o $$(ARDUBOY_BUILDDIR)/$(1).elf \
		$$(filter %.o, $$^) $$(filter %.a, $$+) \
		-lgcc $$(filter-out -Wl%.map, $$(PLUGINLDFLAGS)) -Wl,$$(LDMAP_OPT),$$(ARDUBOY_BUILDDIR)/$(1).map
	$$(SILENT)$$(call objcopy_plugin,$$(ARDUBOY_BUILDDIR)/$(1).elf,$$@)
endef
$(foreach g,$(ARDUBOY_GAMES),$(eval $(call ARDUBOY_GAME_RULE,$(g))))

ARDUBOY_CXXFLAGS = $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign \
	-Wmissing-prototypes -Wno-override-init -Wold-style-declaration,$(PLUGINFLAGS)) \
	-std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	-fno-use-cxa-atexit -fpermissive -w \
	-I$(ARDUBOY_SRCDIR)/lib -I$(ARDUBOY_SRCDIR)

$(ARDUBOY_BUILDDIR)/%.o: $(ARDUBOY_SRCDIR)/%.cpp $(ARDUBOY_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(ARDUBOY_SRCDIR)/,,$<))$(CC) -x c++ $(ARDUBOY_CXXFLAGS) \
	 -I$(dir $<) -c $< -o $@

# Rockbox's C rule compiles ab_glue.c; its header is shared with the C++ code.
$(ARDUBOY_BUILDDIR)/lib/ab_glue.o: $(ARDUBOY_HEADERS)
