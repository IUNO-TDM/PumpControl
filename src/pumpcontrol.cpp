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

      int sflag = 0;
      char *lvalue = NULL;
      char *fvalue = NULL;
      int c;
      opterr = 0;

      /*
        CommandLine args:
        -s:               simulate
        -l [productID]:   the license ID
        -f [serial port]: the serial port of the firmata arduino
      */
      while ((c = getopt (argc, argv, "sl:f:")) != -1)
        switch (c)
          {
          case 's':
            sflag = 1;
            break;
          case 'l':
            lvalue = optarg;
            break;
          case 'f':
            fvalue = optarg;
            printf("%s", optarg);
            break;
          case '?':
            if (optopt == 'l' || optopt == 'f')
              fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
              fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
              fprintf (stderr,
                       "Unknown option character `\\x%x'.\n",
                       optopt);
            return 1;
          default:
            return 1;
            break;
      }

      if(fvalue == NULL){
        fvalue = (char*) &STD_SERIAL_PORT[0];
      }else {
        printf("%s", fvalue);
      }

      PumpControl * pumpControl = new PumpControl(fvalue, lvalue, sflag );
      string jsonText;
      readStdin(&jsonText);
      pumpControl->start(jsonText);
      pumpControl->join();
      delete pumpControl;
      return 0;
    }


    PumpControl::PumpControl(char* serialPort, char* productId, bool simulateMachine)
    {
      initializeIngredientToOutput();
      mSerialPort = serialPort;
      mSimulation = simulateMachine;
      mProductId = productId;

      if(mSimulation){
        printf("The simulation mode is on. Firmata not active!\n");
      }
    }

    PumpControl::PumpControl(){
      PumpControl((char*)STD_SERIAL_PORT[0], NULL, false);
    }

    PumpControl::~PumpControl(){
    }


    void PumpControl::start(string receiptJsonString){
      json j = json::parse(receiptJsonString);
      cout << "Successfully imported receipt: " << j["id"] << endl;
      int maxTime = createTimeProgram(j,mTimeProgram);

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

        if(timing == 0 || timing == 1 || timing == 2){
          for(auto component: line["components"].get<std::vector<json>>())
          {
            string ingredient = component["ingredient"].get<string>();
            int amount = component["amount"].get<int>();
            this->addOutputToTimeProgram(timeProgram,time, mIngredientToOutput[ingredient], true);
            int endTime = time + amount * MS_PER_ML;
            this->addOutputToTimeProgram(timeProgram, endTime, mIngredientToOutput[ingredient], false);
            if (endTime > maxTime){
              maxTime = endTime;
            }
          }

          time = maxTime;
        }else if (timing == 3){
          for(auto component: line["components"].get<std::vector<json>>())
          {
            string ingredient = component["ingredient"].get<string>();
            int amount = component["amount"].get<int>();
            this->addOutputToTimeProgram(timeProgram,time, mIngredientToOutput[ingredient], true);
            int endTime = time + amount * MS_PER_ML;
            this->addOutputToTimeProgram(timeProgram, endTime, mIngredientToOutput[ingredient], false);
            if (endTime > maxTime){
              maxTime = endTime;
            }

            time = maxTime;
          }
        }

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
        }
      }

      timeProgram[time] = timeCommand;
    }

    void PumpControl::timerWorker(int interval, int maximumTime){
        if(!mSimulation){
          mFirmata = firmata_new(mSerialPort); //init Firmata
          while(!mFirmata->isReady) //Wait until device is up
            firmata_pull(mFirmata);

          for(auto i : mPumpToOutput){
            firmata_pinMode(mFirmata, i.second, MODE_OUTPUT);

          }

          this_thread::sleep_for(chrono::seconds(2));
          for(auto i : mPumpToOutput){
            firmata_digitalWrite(mFirmata, i.second, LOW);

          }
        }

        int intervals = maximumTime / interval + 1;
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
            if(!mSimulation){
              firmata_digitalWrite(mFirmata, timeCommand.outputOff[i], LOW);
            }else {
              printf("Simulate Output %d OFF\n", timeCommand.outputOff[i]);
            }
          }

          for(int i = 0; i < timeCommand.onLength; i++){
            if(!mSimulation){
              firmata_digitalWrite(mFirmata, timeCommand.outputOn[i], HIGH);
            }else {
              printf("Simulate Output %d ON\n", timeCommand.outputOn[i]);
            }
          }
       }
    }

    void PumpControl::timerEnded(){
      printf("Timer ended!\n");
      if(!mSimulation){
        for(auto i : mPumpToOutput){
          firmata_digitalWrite(mFirmata, i.second, LOW);
        }
      }

    }
