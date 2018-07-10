#include <signal.h>
#include "generator.h"
#include <wiringPi.h>
#include <mcp23017.h>

//#define FRETS 20
//#define STRINGS 6
//int fretPins[FRETS] = {27,4,5,26,10,11,31,7,0,2,3,12,13,14,30,21,22,23,24,25};
//int stringPins[STRINGS] = {29,28,1,6,9,8};

#define FRETS 21
#define STRINGS 6

#define bus1 100
#define bus2 200

#define SHUTDOWN 300
#define DELAY 10

int fretPins[FRETS] = {bus1+4,bus1+3,bus1+2,bus1+1,bus1,bus2+15,bus2+14,bus2+13,bus2+12,bus2+11,bus2+10,bus2+9,bus2+8,bus2+7,bus2+6,bus2+5,bus2+4,bus2+3,bus2+2,bus2+1,bus2};
int stringPins[STRINGS] = {bus1+10,bus1+11,bus1+12,bus1+13,bus1+14,bus1+15};
int zeroPins[STRINGS] = {29, 28, 25, 24, 22, 23};

bool quit = false;
void siginthandler(int param);
int findPressedFret(int s);

int main( int argc, char *argv[] )
{
    signal(SIGINT, siginthandler);
    generator* gen = new generator();
    std::thread genT(&generator::runLoop, gen);
    genT.detach();

  wiringPiSetup();
  mcp23017Setup (bus1, 0x20) ;
  mcp23017Setup (bus2, 0x21) ;

  for (int s = 0; s < STRINGS; s++)
  {
    pinMode(stringPins[s], OUTPUT);
    digitalWrite(stringPins[s], LOW);
  }

  for (int f = 0; f < FRETS; f++)
  {
    pinMode(fretPins[f], INPUT);
    pullUpDnControl(fretPins[f], PUD_DOWN);
  }

  for (int z = 0; z < STRINGS; z++)
  {
    pinMode(zeroPins[z], INPUT);
    pullUpDnControl(zeroPins[z], PUD_UP);
  }

  int shutdown = SHUTDOWN * 1000 / DELAY;
  int remaining = shutdown;
    while(!quit)
    {
    for(int s = 0; s < STRINGS; s++)
    {
      int pressedFret = -1;
      digitalWrite(stringPins[s], HIGH);
      pressedFret = findPressedFret(s);
      int str = s + 1;
      int fr = pressedFret; 
      if (str < 1 || str > 6)
        continue;
      if (fr < -1 || fr > 21)
        continue;
      gen->setTone(str, fr);
      if(fr > -1)
        remaining = shutdown;
 
      digitalWrite(stringPins[s], LOW);
    }
      std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
      remaining -= DELAY;
      if(remaining <= 0)
      {
	std::cout << "Closed due to inactivity (" << SHUTDOWN << " seconds).\n";
	quit = true;
      }
    }

    for (int s = 0; s < STRINGS; s++)
    {
      digitalWrite(stringPins[s], LOW);
    }
    gen->setQuit();
    if (genT.joinable()) { genT.join(); }
    //if( remaining <= 0)
    //  system("sudo shutdown -h now");
    return 0;
}

int findPressedFret(int s)
{
  int pressedFret = -1;
  if(digitalRead(zeroPins[s]) == LOW)
    pressedFret = 0;
//  else
//    return pressedFret;
  for (int f = 0; f < FRETS; f++)
  {
    int key = FRETS - f;
    if(digitalRead(fretPins[f]) == HIGH)
    {
      return key;
    }
  }
  return pressedFret;
}

void siginthandler(int param)
{
  std::cout << "Closed by user (Ctrl+C).\n";
  quit = true;
}
