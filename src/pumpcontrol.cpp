#include "pumpcontrol.h"
#include <chrono>
using namespace std;
using namespace nlohmann;

    static int readStdin(string *jsonString){
      string line;
      while(getline(cin, line)){
        jsonString->append(line);
      }
      return jsonString->length();
    }

    int main(int argc, char* argv[])
    {
      PumpControl * pumpControl = new PumpControl();
      string jsonText;
      readStdin(&jsonText);
      pumpControl->start(jsonText);
      pumpControl->join();
      delete pumpControl;
      return 0;
    }

    PumpControl::PumpControl(){
      initializeIngredientToOutput();

      // int           i = 0;




      //
      // firmata = firmata_new((char *)&"/dev/tty.usbserial-A104WO1O"[0]); //init Firmata
      // while(!mFirmata->isReady) //Wait until device is up
      //   firmata_pull(mFirmata);
      // firmata_pinMode(mFirmata, 13, MODE_OUTPUT); //set pin 13 (led on most arduino) to out
      // while (1)
      //   {
      //     sleep(1);
      //     if (i++ % 2)
      //       firmata_digitalWrite(mFirmata, 13, HIGH); //light led
      //     else
      //       firmata_digitalWrite(mFirmata, 13, LOW); //unlight led
      //   }
    }
    PumpControl::~PumpControl(){


    }


    void PumpControl::start(string receiptJsonString){
      json j = json::parse(receiptJsonString);
      cout << "Successfully imported receipt: " << j["id"] << endl;
      int maxTime = createTimeProgram(j,mTimeProgram);

      //init firmata


      for(auto i : mTimeProgram){
        printf("time: %d, onLength: %d, offLength: %d\n", i.first, i.second.onLength, i.second.offLength);

      }

      createTimer(1, maxTime);


    }

    void PumpControl::join(){
        if (mTimerThread.joinable()){
          mTimerThread.join();
        }
    }


    void PumpControl::initializeIngredientToOutput(){


      // mIngredientToOutput = new unordered_map<string, int>();
        for(auto i: mPumpToIngredient){

          mIngredientToOutput[i.second] = mPumpToOutput[i.first];
        }
    }


    int PumpControl::createTimeProgram(json j, TimeProgram &timeProgram){


      int time = 0;
      for(auto line: j["lines"].get<vector<json>>())
      {
        int maxTime = time;
        int sleep = line["sleep"].get<int>();
        int timing = line["timing"].get<int>();
        // printf("Line: Sleep: %dms, Timing: %d\n", sleep, timing);
        for(auto component: line["components"].get<std::vector<json>>())
        {
          string ingredient = component["ingredient"].get<string>();
          int amount = component["amount"].get<int>();
          // printf(" %s:%dml -> Output: %d\n",ingredient.c_str(), amount, mIngredientToOutput[ingredient] );

          this->addOutputToTimeProgram(timeProgram,time, mIngredientToOutput[ingredient], true);
          int endTime = time + amount * MS_PER_ML;
          this->addOutputToTimeProgram(timeProgram, endTime, mIngredientToOutput[ingredient], false);
          if (endTime > maxTime){
            maxTime = endTime;
          }
        }

        time = maxTime;
        time += sleep + 1;
      }
      return time;
    }

    void PumpControl::addOutputToTimeProgram(TimeProgram &timeProgram, int time, int output, bool on){
      TsTimeCommand timeCommand = timeProgram[time];


      if(on){
        bool found = false;
        for(int i = 0; i<timeCommand.onLength; i++)
        {
          if(timeCommand.outputOn[i] == output){
            found = true;
            break;
          }
        }
        if(!found){
          timeCommand.onLength++;
          timeCommand.outputOn[timeCommand.onLength-1] = output;
          // printf("Output %d on at %d", output, time);
        }
      }else {
        bool found = false;
        for(int i = 0; i<timeCommand.offLength; i++)
        {
          if(timeCommand.outputOff[i] == output){
            found = true;
            break;
          }
        }
        if(!found){
          timeCommand.offLength++;
          timeCommand.outputOff[timeCommand.offLength-1] = output;
          // printf("Output %d off at %d", output, time);
        }
      }
      timeProgram[time] = timeCommand;

    }

    void PumpControl::timerWorker(int interval, int maximumTime){
        mFirmata = firmata_new((char *)&"/dev/tty.usbserial-A104WO1O"[0]); //init Firmata
        while(!mFirmata->isReady) //Wait until device is up
          firmata_pull(mFirmata);
        for(auto i : mPumpToOutput){
          firmata_pinMode(mFirmata, i.second, MODE_OUTPUT);

        }
        this_thread::sleep_for(chrono::seconds(2));
        for(auto i : mPumpToOutput){
          firmata_digitalWrite(mFirmata, i.second, LOW);

        }

        int intervals = maximumTime / interval + 1;
        // printf("max interval %d\n", intervals);
        auto startTime = chrono::system_clock::now();
        timerFired(0);
        int currentInterval = 1;
        while(currentInterval < intervals){
          this_thread::sleep_until(startTime + chrono::milliseconds(interval * currentInterval));
          timerFired(interval * currentInterval);
          currentInterval++;
        }
        timerEnded();
    }

    void PumpControl::createTimer(int interval, int maximumTime){
      thread myThread ([this, interval, maximumTime]{
        this->timerWorker(interval, maximumTime);
      });
      mTimerThread = move(myThread);

    }

    void PumpControl::timerFired(int time)
    {

      if(mTimeProgram.find(time) != mTimeProgram.end() ){
          TsTimeCommand timeCommand = mTimeProgram[time];
          for(int i = 0; i < timeCommand.offLength; i++){
            firmata_digitalWrite(mFirmata, timeCommand.outputOff[i], LOW);
          }
          for(int i = 0; i < timeCommand.onLength; i++){
            firmata_digitalWrite(mFirmata, timeCommand.outputOn[i], HIGH);
          }
      }
    }

    void PumpControl::timerEnded(){
      printf("Timer ended!\n");
      for(auto i : mPumpToOutput){
        firmata_digitalWrite(mFirmata, i.second, LOW);
      }

    }
