#include <Gamer.h>

Gamer gamer;

static const int trackLength = 1000;

/**
 * How the track is displayed on screen
 */
byte trackParts[4] = {
  B10001111,
  B11000111,
  B11100011,
  B11110001
};

/**
 * Defined allowed transitions from one track part to the next
 */
int trans1[4] = {0,1,1};
int trans2[4] = {0,1,2};
int trans3[4] = {1,2,3};
int trans4[4] = {2,2,3};
int* transitions[4] = {};


byte track[8] = {}; // Remembers the actual deployed track parts
int trackState[8] = {}; // Remembers which transition to apply next (randomly selected)

int gameCount = 0; // Used to make it seem a little random

bool raceInProgress = false; // Is a race running
int raceStartTime; // Start time in milliseconds
int distanceTravelled = 0; // How many track updates have happened

int lastTrackUpdate = 0; // Time of last track update in milliseconds

int dotRacerSpeed = 1000; // Current speed of racer (for track updates)
int dotRacerPosition = 4; // Current X position of racer
int dotRacerFlicker = 1; // toggle to make the dot visible

void setup() {
  Serial.begin(9600);

  // Setup transistions
  transitions[0] = trans1;
  transitions[1] = trans2;
  transitions[2] = trans3;
  transitions[3] = trans4;

  gamer.begin();

  Serial.print("DotRacer Ready...\n");
}

void loop() {
  if ( raceInProgress ) {

    if ( distanceTravelled > trackLength ) {
      double elapsed = ((double)millis() - (double)raceStartTime)/1000;
      for (int i = 0; i<3; i++) {
        gamer.printString("Finished");
      }
      raceInProgress = false;
    } else {
    
      progressTrack();
      updateSpeed();
      updateDotRacer();
      redrawScreen();  
      delay(100); // Sets the flicker rate of the dotRacer
      
    }
    
  } else {
    showSplashScreen();
  }
}

void progressTrack() {
  int now = millis();
    
  if ( (now - lastTrackUpdate) >= dotRacerSpeed ) {
    for (int i = 7; i>0; i--) {
      trackState[i] = trackState[i-1];
      track[i] = track[i-1];
    }
    
    trackState[0] = transitions[trackState[0]][random(0,3)];
    track[0] = trackParts[trackState[0]];
    lastTrackUpdate = now;
    distanceTravelled++;
  }

}

void updateSpeed() {
  if(gamer.isPressed(UP) && dotRacerSpeed > 100) {
      dotRacerSpeed -= 100;
  }
  if(gamer.isPressed(DOWN) && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 100;
  }
  if (racerOffTrack() && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 50;
  }
}

/**
 * Detects "collisions"
 */
int racerOffTrack() {
  byte test = track[7];
  test ^= (-1 ^ test) & (1 << dotRacerPosition);
  if ( test == track[7] ) {
    return 1;
  }
  return 0;
}

void updateDotRacer() {
  if(gamer.isPressed(LEFT) && dotRacerPosition < 7 ) {
    dotRacerPosition += 1;
  }
  if(gamer.isPressed(RIGHT) && dotRacerPosition >= 1) {
    dotRacerPosition -= 1;
  }
  dotRacerFlicker = !dotRacerFlicker;
}

void redrawScreen() {
  byte screen[8] = {};

  // Copy the current state of the track onto the screen
  for (int i = 0; i<=7; i++) { 
    screen[i] = track[i];
  }

  // Place the dotRacer
  screen[7] ^= (-dotRacerFlicker ^ screen[7]) & (1 << dotRacerPosition);
  gamer.printImage(screen);
}

void showSplashScreen() {
  if(gamer.isPressed(START)) {
    
    Serial.print("Race Starting...\n");

    raceInProgress = true;
    raceStartTime = millis();
    distanceTravelled = 0;
    lastTrackUpdate = 0; // reset

    dotRacerSpeed = 1000;
    dotRacerPosition = 4;
    dotRacerFlicker = 1;

    setupTrack();
    gamer.printImage(track);
    
  } else {
    gamer.printString("DOT");
  }
}

void setupTrack() {
  randomSeed(++gameCount);
  int state = random(0,4);
  for (int i = 0; i<8; i++) {
    trackState[i] = state;
    state = transitions[state][random(0,3)];
    track[i] = trackParts[state];
  }
}


