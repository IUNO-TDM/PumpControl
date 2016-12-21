
#include "firmata.h"
#include "json.hpp"
#include <unistd.h>
#include <unordered_map>
#include <thread>


class PumpControl{

  public:
    PumpControl();
    ~PumpControl();
    void start(std::string receiptJsonString);
    void join();

    // static int main(int argc, char* argv[]);

  private:

    typedef struct {
      int onLength = 0;
      int offLength = 0;
      int outputOn[8];
      int outputOff[8];
    }TsTimeCommand;
    typedef std::unordered_map<int,TsTimeCommand> TimeProgram;

    TimeProgram mTimeProgram;


    std::unordered_map<int, std::string> mPumpToIngredient  {
       { 1, "Orangensaft" },
       { 2, "Apfelsaft" },
       { 3, "Sprudel" },
       { 4, "Kirschsaft" },
       { 5, "Bananensaft" },
       { 6, "Johannisbeersaft" },
       { 7, "Cola" },
       { 8, "Fanta" }};
    std::unordered_map<int, int> mPumpToOutput{
          { 1, 3 },
          { 2, 4 },
          { 3, 5 },
          { 4, 6 },
          { 5, 7 },
          { 6, 8 },
          { 7, 9 },
          { 8, 10 }};
    std::unordered_map<std::string, int> mIngredientToOutput;
    const int MS_PER_ML = 700;
    t_firmata     *mFirmata;
    std::thread mTimerThread;

    void initializeIngredientToOutput();


    int createTimeProgram(nlohmann::json j, TimeProgram &timeProgram);
    void addOutputToTimeProgram(TimeProgram &timeProgram, int time, int output, bool on);
    void timerWorker(int interval, int maximumTime);
    void createTimer(int interval, int maximumTime);
    void timerFired( int time);
    void timerEnded();

};
