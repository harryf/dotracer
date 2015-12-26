#include <Gamer.h>

Gamer gamer;

static const int trackLength = 500; // How far down the track do you need to go to finish?
static const int DEBUG = 0; // Serial library seems to leak or cause weirdness so make toggle-able

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

unsigned short gameCount = 0; // Used to make it seem a little random

bool raceInProgress = false; // Is a race running
unsigned long raceStartTime; // Start time in milliseconds
unsigned long lastTrackUpdate = 0; // Time of last track update in milliseconds
unsigned short distanceTravelled = 0; // How many track updates have happened

/**
 * Current 'speed' of racer for track updates. Speed is measured in the number
 * of milliseconds until the next track update so a smaller value for speed means
 * dotRacer is going _faster_. Max speed: 100, min speed: 1000
 */
unsigned short dotRacerSpeed = 1000; // 
unsigned short dotRacerPosition = 4; // Current X position of racer
bool dotRacerFlicker = true; // toggle to make the dot visible

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }

  // Setup transistions
  transitions[0] = trans1;
  transitions[1] = trans2;
  transitions[2] = trans3;
  transitions[3] = trans4;

  gamer.begin();

  if (DEBUG) {
    Serial.print("DotRacer Ready...\n");
  }
}

void loop() {
  if ( raceInProgress ) {

    // End condition...
    if ( distanceTravelled > trackLength ) {

      // What to do with elapsed time? printScreen?
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
    // Move the track down the screen
    for (int i = 7; i>0; i--) {
      trackState[i] = trackState[i-1];
      track[i] = track[i-1];
    }

    // Place a new random track part at the top of screen
    trackState[0] = transitions[trackState[0]][random(0,3)];
    track[0] = trackParts[trackState[0]];
    
    lastTrackUpdate = now;
    distanceTravelled++;
  }

}

void updateSpeed() {
  // Player pressed speed up (up button)
  if(gamer.isPressed(UP) && dotRacerSpeed > 100) {
      dotRacerSpeed -= 100;
  }
  // Player pressed slow down (down button)
  if(gamer.isPressed(DOWN) && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 100;
  }
  // We detected a collision - the racer is off the track
  // so slow it down
  if (racerOffTrack() && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 50;
  }
}

/**
 * Detects "collisions". Combined all the bits set by the track
 * with the racer's bit and test to see if the racer is still
 * "visible". If not, it's off the track
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
  // The flicker helps make the racer visible to the eye even when off the track
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
    startRace();
  } else {
    gamer.printString("DOT");
  }
}

void startRace() {
  if (DEBUG) {
    Serial.print("Race Starting...\n");
  }

  raceInProgress = true;
  raceStartTime = millis();
  distanceTravelled = 0;
  lastTrackUpdate = 0; // reset

  dotRacerSpeed = 1000;
  dotRacerPosition = 4;
  dotRacerFlicker = true;

  setupTrack();
  gamer.printImage(track);
}

void setupTrack() {
  randomSeed(++gameCount*100);
  int state = random(0,4);
  for (int i = 0; i<8; i++) {
    trackState[i] = state;
    state = transitions[state][random(0,3)];
    track[i] = trackParts[state];
  }
}


