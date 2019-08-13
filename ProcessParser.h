#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"


using namespace std;

class ProcessParser{
private:
    std::ifstream stream;
    public:
    static string getCmd(string pid)
    {
        string line ; 
        ifstream stream1 ; 
        Util::getStream((Path::basePath() + pid + Path::cmdPath()), stream1);
        getline(stream1, line);
        return line;
    }
    static vector<string> getPidList()
    {
        vector<string> pidList; 
        DIR *dir; 

        if(!(dir = opendir("/proc")))
            throw runtime_error(strerror(errno));

        while(dirent * dirp = readdir(dir))
        {
            if(dirp->d_type != DT_DIR)
                continue; 
            if(all_of(dirp->d_name, dirp->d_name+strlen(dirp->d_name),[](char c){return isdigit(c);}))
            {   
                pidList.push_back(dirp->d_name);
            }
            
        }
        return pidList;
    }
    static std::string getVmSize(string pid)
    {
        try{
        string line; 
        //string to look for in /proc/pid/status file 
        string name = "VmData"; 
        string value; 
        float result; 

        ifstream stream ;

        //open filestream 
        Util::getStream((Path::basePath() + pid + Path::statusPath()),stream); 

        //read file stream line by line 
        while(getline(stream,line))
        {
            
            //check in each line if our string "VmData" exist 
            if(line.compare(0,name.size(),name) == 0)
            {
                //when we find line with string ""VmData" we read the line string through sstream 
                istringstream buf(line); 

                //reading all strings through a string iterator and adding all the strings to a vector
                istream_iterator<string> beg(buf), end ;
                vector<string> values(beg,end); 

                //The first string element(at 0th index in vector) we know will be "=", and the second element is the data which we convert kB->GB
                result = (stod(values[1])/float(1024*1024));
                break;

            }
        }
        return to_string(result);
        }
        catch(exception& err)
        {

        }
        //convert the result back to string and return
        
    }
    static std::string getCpuPercent(string pid)
    {
        string line ; 
        string  value; 
        float result ; 
        ifstream stream; 
        Util::getStream((Path::basePath()+pid+"/"+Path::statPath()),stream);
        getline(stream, line);
        string str = line; 
        istringstream buf(str); 
        istream_iterator<string>  beg(buf), end; 
        vector<string> values(beg,end); 

        float utime = stof(ProcessParser::getProcUpTime(pid)); 
        float stime = stof(values[14]);
        float cutime = stof(values[15]);
        float cstime = stod(values[16]); 
        float startime = stof(values[21]); 
        float uptime = ProcessParser::getSysUpTime(); 
        float freq = sysconf(_SC_CLK_TCK); 
        float total_time = utime + stime + cutime + cstime;
        float seconds = uptime - (startime/freq); 
        result = 100 * ((total_time/freq)/seconds); 
        return to_string(result);
    }
    static long int getSysUpTime()
    {
        string line ; 
        ifstream stream; 
        Util::getStream((Path::basePath() + Path::upTimePath()),stream);
        getline(stream,line);
        istringstream buf(line); 
        istream_iterator<string> beg(buf), end; 
        vector<string> values(beg,end);
        return stoi(values[0]);
    }
    static std::string getProcUpTime(string pid)
    {
        string line ; 
        string value; 
        float result; 
        ifstream stream ; 
        Util::getStream((Path::basePath() + pid + "/" + Path::statPath()), stream);
        getline(stream, line); 
        string str = line ; 
        istringstream buf(str); 
        istream_iterator<string> beg(buf), end; 

        vector<string> values(beg ,end); 
        // cout<<"uptime : " <<values[13]<<"\n";
        // cout<<"Clk Tick : "<< sysconf(_SC_CLK_TCK) <<"\n";
        // cout<<"TotalProc Up Time : "<<float(stof(values[13])/ sysconf(_SC_CLK_TCK))<<"\n";

        return to_string(float(stof(values[13])/ sysconf(_SC_CLK_TCK)));
    }
    static string getProcUser(string pid)
    {
        string line ; 
        string name = "Uid:"; 
        string result = "";
        ifstream stream; 
        Util::getStream((Path::basePath()+pid+Path::statusPath()),stream);
        
        //get UID for the user 

        while(getline(stream, line))
        {
            if(line.compare(0,name.size(), name)== 0)
            {
               
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg,end); 
                result = values[1]; 
                break ;
            }
        }
        
        Util::getStream("/etc/passwd",stream );
        name =("x:" + result);
        
       //searching for name of uset with selected UID  

       while(getline(stream, line))
       {
           if(line.find(name) != string::npos)
           {
               result = line.substr(0, line.find(":"));
               return result ; 
           }
       }
       
       return "";

    }
    static vector<string> getSysCpuPercent(string coreNumber = "")
    {
        //This is used to read overall cpu percent usage
        // It is possible to use this method for selection of data for overall cpu or every core.
        // when nothing is passed "cpu" line is read
        // when, for example "0" is passed  -> "cpu0" -> data for first core is read

        string line ; 
        string name = "cpu" + coreNumber; 

        ifstream stream ; 
        Util::getStream((Path::basePath()+ Path::statPath()),stream);
        while(getline(stream, line))
        {
            if(line.compare(0,name.size(),name)== 0 )
            {
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg, end); 
                return values; 
            }
        }
        return (vector<string>());
    }
    static float getSysRamPercent()
    {
        string line ; 
        string name1 = "MemAvailable:";
        string name2 = "MemFree:"; 
        string name3 = "Buffers:"; 

        string value; 
        int result; 
        ifstream stream ; 
        Util::getStream((Path::basePath() +Path::memInfoPath()),stream);
        float total_mem = 0; 
        float free_mem = 0; 
        float buffers = 0; 
        while(getline(stream, line))
        {
            if(total_mem != 0 && free_mem != 0)
            {
                break; 
            }

            if(line.compare(0,name1.size(),name1)==0)
            {
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg, end); 
                total_mem = stof(values[1]); 
            }
            if(line.compare(0,name2.size(),name2)==0)
            {
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg, end); 
                free_mem = stof(values[1]); 
            }
            if(line.compare(0,name3.size(),name3)==0)
            {
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg, end); 
                buffers = stof(values[1]); 
            }
        } 
        return float(100*(1-(free_mem/ (total_mem -buffers))));
    }
    static string getSysKernelVersion()
    {
        string line; 
        ifstream stream ; 
        Util::getStream((Path::basePath() + Path::versionPath()),stream);
        getline(stream, line); 
        string str = line;
        istringstream buf(str);
        istream_iterator<string> beg(buf), end; 
        vector<string>values(beg,end); 
        return values[2]; 
    }
    static int getNumberOfCores()
    {
        string line ; 
        string name = "cpu cores"; 
        ifstream stream ; 
        Util::getStream((Path::basePath()+ "cpuinfo"),stream);
        while(getline(stream,line))
        {
            if(line.compare(0,name.size(),name)== 0)
            {
                istringstream buf(line); 
                istream_iterator<string> beg(buf), end; 
                vector<string> values(beg, end); 
                return stoi(values[3]);
            }
        }
        return 0;

    }
    static int getTotalThreads()
    {
        string line;
        int result = 0;
        string name = "Threads:";
        vector<string>_list = ProcessParser::getPidList();
         for (int i=0 ; i<_list.size();i++) 
         {
             string pid = _list[i];
             //getting every process and reading their number of their threads
            ifstream stream ;
            Util::getStream((Path::basePath() + pid + Path::statusPath()),stream);
             while (std::getline(stream, line)) 
             {
                if (line.compare(0, name.size(), name) == 0)
                {
                    istringstream buf(line);
                    istream_iterator<string> beg(buf), end;
                    vector<string> values(beg, end);
                    result += stoi(values[1]);
                    break;
                }
            }
         }
    return result;
    }
    static int getTotalNumberOfProcesses()
    {
        string line;
         int result = 0;
         string name = "processes";
         ifstream stream ;
         Util::getStream((Path::basePath() + Path::statPath()),stream);
         while (std::getline(stream, line)) 
         {
            if (line.compare(0, name.size(), name) == 0) 
            {
                istringstream buf(line);
                istream_iterator<string> beg(buf), end;
                vector<string> values(beg, end);
                result += stoi(values[1]);
                break;
            }
        }
    return result;
    }
    static int getNumberOfRunningProcesses()
    {
        string line;
         int result = 0;
        string name = "procs_running";
        ifstream stream ;
        Util::getStream((Path::basePath() + Path::statPath()),stream);
        while (std::getline(stream, line))
        {
            if (line.compare(0, name.size(), name) == 0) 
            {
                istringstream buf(line);
                istream_iterator<string> beg(buf), end;
                vector<string> values(beg, end);
                result += stoi(values[1]);
                break;
            }
         }
    return result;
    }
    static string getOSName()
    {
        string line;
        string name = "PRETTY_NAME=";

        ifstream stream1 ;
        Util::getStream(("/etc/os-release"), stream1);

        while (std::getline(stream1, line)) 
        {
        if (line.compare(0, name.size(), name) == 0) 
        {
              std::size_t found = line.find("=");
              found++;
              string result = line.substr(found);
              result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
              return result;
        }
        }
    }
    static std::string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2)
    {
        // This method calculates CPU usage, either overall or for a selected core. 
        // The function has two parameters: previous and current time.
        // CPU stats are time-dependent, so the only way to get valid CPU statistics are by specifying a time interval. 
        /*
        Because CPU stats can be calculated only if you take measures in two different time,
        this function has two parameters: two vectors of relevant values.
        We use a formula to calculate overall activity of processor.
        */

       float activeTime = getSysActiveCpuTime(values2) - getSysActiveCpuTime(values1); 
       float idleTime = getSysIdleTime(values2) - getSysIdleTime(values1); 
       float totalTime = activeTime + idleTime; 
       float result = 100 * (activeTime / totalTime); 
       return to_string(result);

    }
    static float getSysActiveCpuTime(vector<string> values)
    {
        return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
    }

    static float getSysIdleTime(vector<string> values)
    {
        return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));

    }
    static bool isPidExisting(string pid)
    {
       bool result = false; 
       vector<string> list = ProcessParser::getPidList(); 
       for(int i =0; i <list.size(); i++)
       {
           if(pid == list[i])
           {
               result = true; 
               break;
           }
       }
       return result;
    }
};

// TODO: Define all of the above functions below:
