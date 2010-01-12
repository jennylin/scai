#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <windows.h>

static bool analyzed;
static bool analysis_just_finished;
static BWTA::Region* home;
static BWTA::Region* enemy_base;
DWORD WINAPI AnalyzeThread();

class ExampleAIModule : public BWAPI::AIModule
{	

public:
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual bool onSendText(std::string text);
  virtual void onPlayerLeft(BWAPI::Player* player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitCreate(BWAPI::Unit* unit);
  virtual void onUnitDestroy(BWAPI::Unit* unit);
  virtual void onUnitMorph(BWAPI::Unit* unit);
  virtual void onUnitShow(BWAPI::Unit* unit);
  virtual void onUnitHide(BWAPI::Unit* unit);
  virtual void onUnitRenegade(BWAPI::Unit* unit);
  void drawStats(); //not part of BWAPI::AIModule
  void showPlayers();
  void showForces();
  bool show_visibility_data;

private: //none are part of BWAPI::AIModule
BWAPI::Position tilePositionToPosition(BWAPI::TilePosition tp);
BWAPI::TilePosition positionToTilePosition(BWAPI::Position p);
BWAPI::Position getAvgPosition(BWAPI::Player *p);
  BWAPI::Position getPointReflection(BWAPI::Position p, BWAPI::Position a); // find p' from p about a
  BWAPI::Position getDoublePointReflection(BWAPI::Position p, BWAPI::Position a); // find p' from p about a
  BWAPI::Position valid(BWAPI::Position from, BWAPI::Position to);
  void getNearbyUnits(std::set<BWAPI::Unit*> units,BWAPI::TilePosition p, std::set<BWAPI::Unit*>& nearbyUnits);
  BWAPI::Position huntEnemies();
  void callJennyCode();
  bool close(BWAPI::Position p1, BWAPI::Position p2);
  bool close(BWAPI::TilePosition p1, BWAPI::TilePosition p2);
  BWAPI::Position calcCurrentAim();
  
  int currentAimPos;
  int startPos;
  int circleCount;
  int direction;

  int mapHeight;
  int mapWidth;
};