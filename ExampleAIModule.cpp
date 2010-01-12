#include "ExampleAIModule.h"
#include "Constants.h"
using namespace BWAPI;
void ExampleAIModule::onStart()
{
  mapHeight = Broodwar->mapHeight() * TILE_SIZE;
  mapWidth = Broodwar->mapWidth() * TILE_SIZE;
  TilePosition startLocation = Broodwar->self()->getStartLocation();
  if (startLocation.x() < mapWidth/2) {
	  currentAimPos = POS_NOON;
	  startPos = POS_NINE;
  } else {
	  currentAimPos = POS_SIX;
	  startPos = POS_THREE;
  }
  Broodwar->printf("map size is %d %d", mapWidth, mapHeight);
  Broodwar->printf("startPos is %d and currentAimPos is %d", startPos, currentAimPos);
  circleCount = 1;
  direction = CIRCLE_IN;
  Broodwar->sendText("Hello world!");
  Broodwar->printf("The map is %s, a %d player map",Broodwar->mapName().c_str(),Broodwar->getStartLocations().size());
  // Enable some cheat flags
  Broodwar->enableFlag(Flag::UserInput);
  // Uncomment to enable complete map information
  //Broodwar->enableFlag(Flag::CompleteMapInformation);

  //read map information into BWTA so terrain analysis can be done in another thread
  BWTA::readMap();
  analyzed=false;
  analysis_just_finished=false;
  show_visibility_data=false;

  if (Broodwar->isReplay())
  {
    Broodwar->printf("The following players are in this replay:");
    for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++)
    {
      if (!(*p)->getUnits().empty() && !(*p)->isNeutral())
      {
        Broodwar->printf("%s, playing as a %s",(*p)->getName().c_str(),(*p)->getRace().getName().c_str());
      }
    }
  }
  else
  {
    Broodwar->printf("The match up is %s v %s",
      Broodwar->self()->getRace().getName().c_str(),
      Broodwar->enemy()->getRace().getName().c_str());

    //send each worker to the mineral field that is closest to it
    for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
    {
      if ((*i)->getType().isWorker())
      {
        Unit* closestMineral=NULL;
        for(std::set<Unit*>::iterator m=Broodwar->getMinerals().begin();m!=Broodwar->getMinerals().end();m++)
        {
          if (closestMineral==NULL || (*i)->getDistance(*m)<(*i)->getDistance(closestMineral))
            closestMineral=*m;
        }
        if (closestMineral!=NULL)
          (*i)->rightClick(closestMineral);
      }
      else if ((*i)->getType().isResourceDepot())
      {
        //if this is a center, tell it to build the appropiate type of worker
        (*i)->train(*Broodwar->self()->getRace().getWorker());
      }
    }
  }
}
void ExampleAIModule::onEnd(bool isWinner)
{
  if (isWinner)
  {
    //log win to file
  }
}

void ExampleAIModule::onFrame()
{
	Broodwar->drawCircle(CoordinateType::Map,mapWidth/2, mapHeight/2,20,Colors::Green,true);
  if (show_visibility_data)
  {    
    for(int x=0;x<Broodwar->mapWidth();x++)
    {
      for(int y=0;y<Broodwar->mapHeight();y++)
      {
        if (Broodwar->isExplored(x,y))
        {
          if (Broodwar->isVisible(x,y))
            Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Green);
          else
            Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Blue);
        }
        else
          Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Red);
      }
    }
  }

  if (Broodwar->isReplay())
    return;

  drawStats();
  if (analyzed && Broodwar->getFrameCount()%30==0)
  {
    //order one of our workers to guard our chokepoint.
    for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
    {
      if ((*i)->getType().isWorker())
      {
        //get the chokepoints linked to our home region
        std::set<BWTA::Chokepoint*> chokepoints= home->getChokepoints();
        double min_length=10000;
        BWTA::Chokepoint* choke=NULL;

        //iterate through all chokepoints and look for the one with the smallest gap (least width)
        for(std::set<BWTA::Chokepoint*>::iterator c=chokepoints.begin();c!=chokepoints.end();c++)
        {
          double length=(*c)->getWidth();
          if (length<min_length || choke==NULL)
          {
            min_length=length;
            choke=*c;
          }
        }

        //order the worker to move to the center of the gap
        (*i)->rightClick(choke->getCenter());
        break;
      }
    }
  }
  if (analyzed)
  {
    //we will iterate through all the base locations, and draw their outlines.
    for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
    {
      TilePosition p=(*i)->getTilePosition();
      Position c=(*i)->getPosition();

      //draw outline of center location
      Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);

      //draw a circle at each mineral patch
      for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
      {
        Position q=(*j)->getInitialPosition();
        Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
      }

      //draw the outlines of vespene geysers
      for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
      {
        TilePosition q=(*j)->getInitialTilePosition();
        Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
      }

      //if this is an island expansion, draw a yellow circle around the base location
      if ((*i)->isIsland())
      {
        Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
      }
    }
    
    //we will iterate through all the regions and draw the polygon outline of it in green.
    for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
    {
      BWTA::Polygon p=(*r)->getPolygon();
      for(int j=0;j<(int)p.size();j++)
      {
        Position point1=p[j];
        Position point2=p[(j+1) % p.size()];
        Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
      }
    }

    //we will visualize the chokepoints with red lines
    for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
    {
      for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
      {
        Position point1=(*c)->getSides().first;
        Position point2=(*c)->getSides().second;
        Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
      }
    }
  }
  if (analysis_just_finished)
  {
    Broodwar->printf("Finished analyzing map.");
    analysis_just_finished=false;
  }
  
	callJennyCode();
  
}

void ExampleAIModule::onUnitCreate(BWAPI::Unit* unit)
{/*
  if (!Broodwar->isReplay())
    Broodwar->sendText("A %s [%x] has been created at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
  else
  {
    //if we are in a replay, then we will print out the build order
    //(just of the buildings, not the units).
    if (unit->getType().isBuilding() && unit->getPlayer()->isNeutral()==false)
    {
      int seconds=Broodwar->getFrameCount()/24;
      int minutes=seconds/60;
      seconds%=60;
      Broodwar->sendText("%.2d:%.2d: %s creates a %s",minutes,seconds,unit->getPlayer()->getName().c_str(),unit->getType().getName().c_str());
    }
  }*/
}
void ExampleAIModule::onUnitDestroy(BWAPI::Unit* unit)
{
  /*
  if (!Broodwar->isReplay())
    Broodwar->sendText("A %s [%x] has been destroyed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
  */
}

void ExampleAIModule::onUnitMorph(BWAPI::Unit* unit)
{
  if (!Broodwar->isReplay())
    Broodwar->sendText("A %s [%x] has been morphed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
  else
  {
    /*if we are in a replay, then we will print out the build order
    (just of the buildings, not the units).*/
    if (unit->getType().isBuilding() && unit->getPlayer()->isNeutral()==false)
    {
      int seconds=Broodwar->getFrameCount()/24;
      int minutes=seconds/60;
      seconds%=60;
      Broodwar->sendText("%.2d:%.2d: %s morphs a %s",minutes,seconds,unit->getPlayer()->getName().c_str(),unit->getType().getName().c_str());
    }
  }
}
void ExampleAIModule::onUnitShow(BWAPI::Unit* unit)
{/*
  if (!Broodwar->isReplay())
    Broodwar->sendText("A %s [%x] has been spotted at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
*/
}
void ExampleAIModule::onUnitHide(BWAPI::Unit* unit)
{
 // if (!Broodwar->isReplay())
   // Broodwar->sendText("A %s [%x] was last seen at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x(),unit->getPosition().y());
}
void ExampleAIModule::onUnitRenegade(BWAPI::Unit* unit)
{
  if (!Broodwar->isReplay())
    Broodwar->sendText("A %s [%x] is now owned by %s",unit->getType().getName().c_str(),unit,unit->getPlayer()->getName().c_str());
}
void ExampleAIModule::onPlayerLeft(BWAPI::Player* player)
{
  Broodwar->sendText("%s left the game.",player->getName().c_str());
}
void ExampleAIModule::onNukeDetect(BWAPI::Position target)
{
  if (target!=Positions::Unknown)
    Broodwar->printf("Nuclear Launch Detected at (%d,%d)",target.x(),target.y());
  else
    Broodwar->printf("Nuclear Launch Detected");
}

bool ExampleAIModule::onSendText(std::string text)
{
  if (text=="/show players")
  {
    showPlayers();
    return false;
  } else if (text=="/show forces")
  {
    showForces();
    return false;
  } else if (text=="/show visibility")
  {
    show_visibility_data=true;
  } else if (text=="/analyze")
  {
    if (analyzed == false)
    {
      Broodwar->printf("Analyzing map... this may take a minute");
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);
    }
    return false;
  } else
  {
    Broodwar->printf("You typed '%s'!",text.c_str());
  }
  return true;
}

DWORD WINAPI AnalyzeThread()
{
  BWTA::analyze();
  analyzed   = true;
  analysis_just_finished = true;

  //self start location only available if the map has base locations
  if (BWTA::getStartLocation(BWAPI::Broodwar->self())!=NULL)
  {
    home       = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
  }
  //enemy start location only available if Complete Map Information is enabled.
  if (BWTA::getStartLocation(BWAPI::Broodwar->enemy())!=NULL)
  {
    enemy_base = BWTA::getStartLocation(BWAPI::Broodwar->enemy())->getRegion();
  }
  return 0;
}

void ExampleAIModule::drawStats()
{
  std::set<Unit*> myUnits = Broodwar->self()->getUnits();
  Broodwar->drawTextScreen(5,0,"I have %d units:",myUnits.size());
  std::map<UnitType, int> unitTypeCounts;
  int line=1;
  for(std::set<Unit*>::iterator i=myUnits.begin();i!=myUnits.end();i++)
  {
	Broodwar->drawTextScreen(5,16*line,"position (%d, %d)",(*i)->getTilePosition().x(),(*i)->getTilePosition().y());
    line++;
	if (unitTypeCounts.find((*i)->getType())==unitTypeCounts.end())
    {
      unitTypeCounts.insert(std::make_pair((*i)->getType(),0));
    }
    unitTypeCounts.find((*i)->getType())->second++;
  }/*
  int line=1;
  for(std::map<UnitType,int>::iterator i=unitTypeCounts.begin();i!=unitTypeCounts.end();i++)
  {
    Broodwar->drawTextScreen(5,16*line,"- %d %ss",(*i).second, (*i).first.getName().c_str());
    line++;
  }*/
}


void ExampleAIModule::showPlayers()
{
  std::set<Player*> players=Broodwar->getPlayers();
  for(std::set<Player*>::iterator i=players.begin();i!=players.end();i++)
  {
    Broodwar->printf("Player [%d]: %s is in force: %s",(*i)->getID(),(*i)->getName().c_str(), (*i)->getForce()->getName().c_str());
  }
}
void ExampleAIModule::showForces()
{
  std::set<Force*> forces=Broodwar->getForces();
  for(std::set<Force*>::iterator i=forces.begin();i!=forces.end();i++)
  {
    std::set<Player*> players=(*i)->getPlayers();
    Broodwar->printf("Force %s has the following players:",(*i)->getName().c_str());
    for(std::set<Player*>::iterator j=players.begin();j!=players.end();j++)
    {
      Broodwar->printf("  - Player [%d]: %s",(*j)->getID(),(*j)->getName().c_str());
    }
  }
}

Position ExampleAIModule::getAvgPosition(Player *p)
{
	int count = 0;
	int x = 0;
	int y = 0;
	if (p != NULL) {
		for(std::set<Unit*>::const_iterator i=p->getUnits().begin();i!=p->getUnits().end();i++)
		{
			count++;
			x += (*i)->getPosition().x();
			y += (*i)->getPosition().y();
		}
	}
	if (count > 0) {
		x = x/count;
		y = y/count;
	} else {
		x = Broodwar->mapWidth()*TILE_SIZE/2;
		y = Broodwar->mapHeight()*TILE_SIZE/2;
	}

	return Position(x,y);
}


Position ExampleAIModule::tilePositionToPosition(TilePosition tp) 
{
	return Position(tp.x()*TILE_SIZE, tp.y()*TILE_SIZE);
}

TilePosition ExampleAIModule::positionToTilePosition(Position p) {
	return Position(p.x()/TILE_SIZE, p.y()/TILE_SIZE);
}

void ExampleAIModule::getNearbyUnits(std::set<Unit*> units, TilePosition p, std::set<Unit*>& nearbyUnits)
{
	for (std::set<Unit*>::const_iterator it = units.begin(); it != units.end(); it++) {
		if (close((*it)->getTilePosition(), p)) {
			nearbyUnits.insert(*it);
		}
	}
}

bool ExampleAIModule::close(Position p1, Position p2)
{
	return close(positionToTilePosition(p1), positionToTilePosition(p2));
}

bool ExampleAIModule::close(TilePosition p1, TilePosition p2)
{
	return ((abs(p1.x() - p2.x()) < 1) && (abs(p1.y() - p2.y()) < 1));
}

 // find p' from p about a
Position ExampleAIModule::getPointReflection(Position p, Position a)
{
	return Position(a.x()*2-p.x(), a.y()*2-p.y());
}

// find p' from p about a twice distance from a
Position ExampleAIModule::getDoublePointReflection(Position p, Position a)
{
	return Position(a.x()*3-p.x()*2, a.y()*3-p.y()*2);
}

Position ExampleAIModule::valid(Position from, Position to)
{
	TilePosition toTile = positionToTilePosition(to);
	if (toTile.isValid())
	{
		return to;
	}

	TilePosition fromTile = positionToTilePosition(from);
	int count=0;
	int diffx=1;
	int diffy=1;
	Position newTo;
	do
	{
		count -= TILE_SIZE;
		if (from.x() == to.x()) {
			if (from.y() > to.y())
			{
				diffy = -1;
			}
			newTo = Position(to.x(), diffy*count + to.y());
		}
		else
		{
			if (abs(from.x()-to.x()) > abs(from.y()-to.y()))
			{
				diffx=(to.x()-from.x())/(to.y()-from.y());
			}
			else
			{
				diffy=(to.y()-from.y())/(to.y()-from.y());
			}
			newTo = Position(diffx*count + to.x(), diffy*count + to.y());
		}
	} while (!(positionToTilePosition(newTo)).isValid());

	return newTo;
}

Position ExampleAIModule::calcCurrentAim()
{
	return Position(mapWidth/2+pos_x[currentAimPos]*(mapWidth/2-20*TILE_SIZE*circleCount), mapHeight/2+pos_y[currentAimPos]*(mapHeight/2-20*TILE_SIZE*circleCount));
}


// send them in a concentric circle counter clockwise around the map
Position ExampleAIModule::huntEnemies() 
{
	// if we are in the middle
	if (getAvgPosition(Broodwar->self()).getDistance(Position(mapWidth/2, mapHeight/2)) < TILE_SIZE) {
		Broodwar->printf("in the middle");	
		direction *= -1;
	}
	if (getAvgPosition(Broodwar->self()).getDistance(calcCurrentAim()) < 15*TILE_SIZE) {
		currentAimPos = ++currentAimPos % 4;
		Broodwar->printf("switching aim");
	}

	if (currentAimPos == startPos) {
		Broodwar->printf("changing circle size");
		circleCount += direction;
	}

	return calcCurrentAim();
}

void ExampleAIModule::callJennyCode()
{
	// if no enemies we march for middle
	Position enemyCenter;
	if ((Broodwar->enemy() == NULL) || (Broodwar->enemy()->getUnits().size() == 0)) {
		enemyCenter = huntEnemies();
		Broodwar->drawCircle(CoordinateType::Map,enemyCenter.x(), enemyCenter.y(),20,Colors::Yellow,true);
	} else {
		enemyCenter = getAvgPosition(Broodwar->enemy());
	}
	int unitnum=-1;
	//Broodwar->printf("going for %d %d", enemyCenter.x(), enemyCenter.y());
	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
    {
		unitnum++;
		// first prioriy, if less than 2 strikes away from death, run away

		// otherwise, if there are no enemies nearby, get yourself to the center of the action
		Position position = (*i)->getPosition();
		std::set<Unit*> closeEnemyUnits;
		if (Broodwar->enemy() == NULL) {
			Broodwar->printf("unit %d is attack moving to %d %d",unitnum, enemyCenter.x(), enemyCenter.y());
			(*i)->attackMove(valid(position, enemyCenter));
			continue;
		}
		
		getNearbyUnits(Broodwar->enemy()->getUnits(), (*i)->getTilePosition(), closeEnemyUnits);
		
		if (closeEnemyUnits.empty()) {
			Position goTo = getPointReflection(position, enemyCenter);
			if (Broodwar->getFrameCount()%45==0) {
				Broodwar->printf("unit %d attack moving to %d %d",unitnum, goTo.x(), goTo.y());
				(*i)->attackMove(valid(position, goTo));
			}
			continue;
		}

		// if there are enemies nearby, attack the one with the lowest hitpoints
		Unit *lowest = NULL;
		for each (Unit *u in closeEnemyUnits) {
			if (lowest == NULL) {
				lowest = u;
			} else {
				if (u->getHitPoints()+u->getShields() < lowest->getHitPoints()+u->getShields()) {
					lowest = u;
				}
			}
		}
		if (((*i)->getGroundWeaponCooldown() > 15) && (lowest != (*i)->getTarget()))
		{
			Broodwar->printf("unit %d changing who to attack", unitnum);
			(*i)->attackUnit(lowest);
		}
		continue;
	
	}
}