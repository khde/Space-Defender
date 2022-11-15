#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define TITEL "Space Defender"
#define FENSTERH 700
#define FENSTERB 1100
#define MPB 16.66f // ms pro Frame, entspricht 60 FPS

#define SPIELERPROJEKTILV 12
#define SPIELERV 4
#define SPIELERSCHADEN 50

#define GEGNERLEBEN 40
#define GEGNERV 3
#define GEGNERREIHEN 6 // Nein, Gegnerspalten
#define GEGNERSPALTEN 3 // Nein Gegnerreihen
#define GEGNERABSTAND 80

#define MAXPROJEKTILE 2048
#define RAUMSCHIFFH 60
#define RAUMSCHIFFB 60
#define LASERH 10
#define LASERB 5

#define RANDABSTAND 20


typedef struct {
    int x;
    int y;
} Position;

// Nicht die beste Idee, oder?
typedef struct {
    bool links;
    bool rechts;
} Richtung;

typedef struct {
    Position position;
    Richtung richtung;
    int leben;
    SDL_Texture *textur;
} Raumschiff;

typedef struct {
    Position position;
    Richtung richtung;
    SDL_Texture *textur;
    Raumschiff gegner[GEGNERSPALTEN][GEGNERREIHEN];
    unsigned int maxGegner;
    int hoehe;
    int breite;
} GegnerStruktur;

typedef struct {
    Position position;
    int geschwindigkeit;
} Laser;

typedef struct {
    bool verloren;
    bool gewonnen;
} SpielStatus;


bool init_spiel();
void game_loop();
void zeichnen();
void eingabe();
void logik();
void bild_rendern(SDL_Texture *textur, const Position *position, int h, int b);
void gegnerStruktur_erzeugen();
void gegnerStruktur_verschieben(int x, int y);
void laser_erzeugen(int x, int y, int v);
bool rechteckkollision(int x1, int y1, int h1, int b1, int x2, int y2, int h2, int b2);
bool laser_gegner_kollision(Raumschiff *raumschiff, Laser *laser);

// Globalität, warum auch an Funktionen übergeben? :D
SDL_Window *fenster;
SDL_Renderer *renderer;
SDL_Event event;
TTF_Font *schrift1;
SDL_Texture *text_gewonnen;
SDL_Texture *text_verloren;
SDL_Color hell = {230, 230, 230};

Raumschiff spieler;
GegnerStruktur gegnerStruktur;
SDL_Texture *laserTextur;
Laser laserStruct;
Laser *laserSpieler = NULL;

bool aktiv;
SpielStatus spielStatus;


int main(int argc, char *argv[]) {
   if (!init_spiel()) {
        return EXIT_FAILURE;
    }

    game_loop();

    TTF_CloseFont(schrift1);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(fenster);
    SDL_Quit();

    return EXIT_SUCCESS;
}


bool init_spiel() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "Fehler: %s\n", SDL_GetError());
        return 0; // 0 bedeutet Fehler
    }

    fenster = SDL_CreateWindow(TITEL,
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            FENSTERB,
                            FENSTERH,
                            SDL_WINDOW_SHOWN);

    if (!fenster) {
        fprintf(stderr, "Fehler beim Fenster: %s\n", SDL_GetError());
        return 0; // 0 bedeutet Fehler
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "Fehler bei TTF_Init: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
    schrift1 = TTF_OpenFont("ressourcen/arial.ttf", 25);

    // Dumm, dass ich die Texturen in ein Struct speichere, aber
    // anstatt es zu ändern schreibe ich diesen Kommentar
    spieler.textur = IMG_LoadTexture(renderer, "ressourcen/spieler.png");
    gegnerStruktur.textur = IMG_LoadTexture(renderer, "ressourcen/gegner.png");
    laserTextur = IMG_LoadTexture(renderer, "ressourcen/laser.png");

    // Text
    SDL_Surface *text;
    text = TTF_RenderText_Solid(schrift1, "Gewonnen!", hell);
    text_gewonnen = SDL_CreateTextureFromSurface(renderer, text);

    text = TTF_RenderText_Solid(schrift1, "Verloren!", hell);
    text_verloren = SDL_CreateTextureFromSurface(renderer, text);

    printf("Fehler: %s\n", SDL_GetError());

    spieler.position.x = FENSTERB / 2;
    spieler.position.y = FENSTERH - 100;
    spieler.richtung.links = false;
    spieler.richtung.rechts = false;

    gegnerStruktur_erzeugen();

    aktiv = true;
    spielStatus.verloren = false;
    spielStatus.gewonnen = false;

    return true;
}


void game_loop() {
    float tDelta;
    Uint64 tStart, tStop;
    while(aktiv) {
        tStart = SDL_GetPerformanceCounter();
        zeichnen();
        eingabe();
        logik();
        tStop = SDL_GetPerformanceCounter();
        zeichnen();
        tDelta = (tStop - tStart) / (float) SDL_GetPerformanceCounter() * 1000.0;
        SDL_Delay(floor(MPB - tDelta));
    }
}


void zeichnen() {
    SDL_RenderClear(renderer); // Hintergrundfarbe
    bild_rendern(spieler.textur, &spieler.position, RAUMSCHIFFH, RAUMSCHIFFB);

    for (int h = 0; h < GEGNERSPALTEN; h++) {
        for (int k = 0; k < GEGNERREIHEN; k++) {
            if (gegnerStruktur.gegner[h][k].leben > 0) {
                bild_rendern(gegnerStruktur.textur, &gegnerStruktur.gegner[h][k].position, RAUMSCHIFFH, RAUMSCHIFFB);
            }
        }
    }

    if (laserSpieler != NULL)  {
        bild_rendern(laserTextur, &laserSpieler->position, LASERH, LASERB);
    }

    if (spielStatus.gewonnen) {
        SDL_RenderCopy(renderer, text_gewonnen, NULL, NULL);
    }
    else if (spielStatus.verloren) {
        SDL_RenderCopy(renderer, text_verloren, NULL, NULL);
    }

    SDL_RenderPresent(renderer);
}


void eingabe() {
    if(SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            aktiv = false;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_q:
                    aktiv = false;
                    break;
                case SDLK_LEFT:
                case SDLK_a:
                    spieler.richtung.links = true;
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    spieler.richtung.rechts = true;
                    break;
                case SDLK_SPACE:
                    if (laserSpieler == NULL) {
                        laser_erzeugen(spieler.position.x + RAUMSCHIFFB / 2, spieler.position.y, SPIELERPROJEKTILV);
                    }
                default:
                    break;
            }
        }
        else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_a:
                    spieler.richtung.links = false;
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    spieler.richtung.rechts = false;
                    break;
                case SDLK_SPACE:
                    break;
                default:
                    break;
            }
        }
    }
}


void logik() {
    int dx = 0;
    if (!(spieler.richtung.links && spieler.richtung.rechts)) {
        if (spieler.richtung.links) {
            dx -= SPIELERV;
        }
        if (spieler.richtung.rechts) {
            dx += SPIELERV;
        }

        if (spieler.position.x + dx < RANDABSTAND) {
            dx = spieler.position.x + dx - RANDABSTAND;
        }
        else if (spieler.position.x + RANDABSTAND + RAUMSCHIFFB + dx > FENSTERB) {
            dx = FENSTERB - (spieler.position.x + RANDABSTAND + RAUMSCHIFFB);
        }
        else {
            spieler.position.x += dx;
        }
    }

    if (gegnerStruktur.richtung.rechts) {
        if (gegnerStruktur.position.x + gegnerStruktur.breite + GEGNERV > FENSTERB) {
            gegnerStruktur.richtung.rechts = false;
            gegnerStruktur.richtung.links = true;
            gegnerStruktur_verschieben(0, RAUMSCHIFFH);
        }
        else {
            gegnerStruktur_verschieben(GEGNERV, 0);
        }
    }
    else if (gegnerStruktur.richtung.links) {
        if (gegnerStruktur.position.x - GEGNERV < RANDABSTAND) {
            gegnerStruktur.richtung.rechts = true;
            gegnerStruktur.richtung.links = false;
            gegnerStruktur_verschieben(0, RAUMSCHIFFH);
        }
        else {
            gegnerStruktur_verschieben(-GEGNERV, 0);
        }
    }

    // Ist der Laser außerhalb des Fensters, nur in -y-Richtung
    if (laserSpieler != NULL) {
        if (laserSpieler->position.y < 0) {
            laserSpieler = NULL;
        }
    }
    if(laserSpieler != NULL) {
        laserSpieler->position.y -= SPIELERPROJEKTILV;
    }

    // Wenn Spielstatus feststeht, überspringe kollisionen, etc. und rendere nur Nachricht
    if (spielStatus.verloren || spielStatus.gewonnen) {
        goto spielEnde;
    }


    // Gegner-Laser kollision
    for (int h = 0; h < GEGNERSPALTEN; h++) {
        for (int k = 0; k < GEGNERREIHEN; k++) {
            if (laserSpieler != NULL) {
                if (gegnerStruktur.gegner[h][k].leben > 0) {
                    if (laser_gegner_kollision(&(gegnerStruktur.gegner[h][k]), laserSpieler)) {
                       gegnerStruktur.gegner[h][k].leben -= SPIELERSCHADEN;
                       laserSpieler = NULL;
                    }
                }
                else {
                    continue;
                }
            }
        }
    }

    // Game Over -> Ein Gegner ist beim Spieler angekommen
    bool gegnerAmLeben = false;
    for (int h = 0; h < GEGNERSPALTEN; h++) {
        for (int k = 0; k < GEGNERREIHEN; k++) {
            if (gegnerStruktur.gegner[h][k].leben > 0) {
                gegnerAmLeben = true;
                if (gegnerStruktur.gegner[h][k].position.y + RAUMSCHIFFH > spieler.position.y) {
                    spielStatus.verloren = true;
                    goto spielEnde;
                }
            }
        }
    }

    if (!gegnerAmLeben) {
        spielStatus.gewonnen = true;
    }

spielEnde:
    ;
}


void bild_rendern(SDL_Texture *textur, const Position *position, int h, int b) {
    SDL_Rect zielrechteck; // 60 px für bild
    zielrechteck.w = b;
    zielrechteck.h = h;
    zielrechteck.x = position->x;
    zielrechteck.y = position->y;
    SDL_RenderCopy(renderer, textur, NULL, &zielrechteck); // Rendert immer ganzes Bild
}

// Funktion hat höhe und breite schlecht umgesetzt :(
void gegnerStruktur_erzeugen() {
    gegnerStruktur.position.x = 100;
    gegnerStruktur.position.y = 40;
    gegnerStruktur.richtung.links = false;
    gegnerStruktur.richtung.rechts = true;
    gegnerStruktur.maxGegner = GEGNERSPALTEN * GEGNERREIHEN;

    int x = gegnerStruktur.position.x;
    int y = gegnerStruktur.position.y;
    for (int h = 0; h < GEGNERSPALTEN; h++) {
        for (int k = 0; k < GEGNERREIHEN; k++) {
            gegnerStruktur.gegner[h][k].position.x = x;
            gegnerStruktur.gegner[h][k].position.y = y;
            gegnerStruktur.gegner[h][k].leben = 100;
            x += GEGNERABSTAND;
        }
        gegnerStruktur.breite = x += 60;
        y += GEGNERABSTAND;
        x = gegnerStruktur.position.x;
    }
    gegnerStruktur.breite -= gegnerStruktur.position.x;
    gegnerStruktur.hoehe = GEGNERSPALTEN * GEGNERABSTAND;
}

// Ich glaube die Funktion ist unnötig, da sich die gegnerischen Raumschiffe sowieso,
// an den Koordinaten der GegnerStruktur ausrichten ????1?!?!!!
void gegnerStruktur_verschieben(int x, int y) {
    gegnerStruktur.position.x += x;
    gegnerStruktur.position.y += y;
    for (int h = 0; h < GEGNERSPALTEN; h++) {
        for (int k = 0; k < GEGNERREIHEN; k++) {
            gegnerStruktur.gegner[h][k].position.x += x;
            gegnerStruktur.gegner[h][k].position.y += y;
        }
    }
}


void laser_erzeugen(int x, int y, int v) {
    laserSpieler = &laserStruct;
    (*laserSpieler).position.x = x;
    (*laserSpieler).position.y = y;
    // laserSpieler.geschwindigkeit = SPIELERPROJEKTILV;  // Ist egal
}


bool rechteckkollision(int x1, int y1, int h1, int b1, int x2, int y2, int h2, int b2) {
    if (x1 > x2 + b2 || x1 + b1 < x2 || y1  > y2 + h2 || y1 + h1 < y2) {return false;}
    return true;
}


bool laser_gegner_kollision(Raumschiff *raumschiff, Laser *laser) {
    return rechteckkollision(
            raumschiff->position.x,
            raumschiff->position.y,
            RAUMSCHIFFH, // Croissant
            RAUMSCHIFFB,
            laser->position.x,
            laser->position.y,
            LASERH,
            LASERB
            );
}
