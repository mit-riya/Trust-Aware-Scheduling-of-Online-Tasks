#include <bits/stdc++.h>
#include <thread>
#include <fstream>
#include <iostream>
using namespace std;
typedef long long llint;

#define meanArrivalRate 5.0
int numberOfUsers = 10;
#define maxExecutionTime 50
int K= 500;
int numTasksPerUser = 100;
#define generateDifferentCodeEverytime 0 // 0: Same data in every run 1: Different data in every run
#define MaxTime 1e8
double init_failure_prob = 0.001;
int heuristic_num = 0;
int numberOfMachines = 10;
default_random_engine generator;
std::ofstream outputFile("output.txt", std::ios::out);
int start = 5;
int stop = 55;
int step = 5;
vector<vector<double>> data;

class Task
{
public:
    int arrivalTime;       // Arrival time of the task
    int executionTime;     // Execution time of the task
    int startingTime = -1; // Starting time of a task
    int finishingTime;
    int userInformation;  // User information associated with the task
    int relativeDeadline; // Relative deadline of the task
    double reliability;

    Task(int arr, int exec, int user, int deadline, double reliability) : arrivalTime(arr), executionTime(exec), userInformation(user), relativeDeadline(deadline), reliability(reliability) {}
    bool operator==(const Task &other) const
    {
        return arrivalTime == other.arrivalTime &&
               executionTime == other.executionTime &&
               startingTime == other.startingTime &&
               finishingTime == other.finishingTime &&
               userInformation == other.userInformation &&
               relativeDeadline == other.relativeDeadline &&
               reliability == other.reliability;
    }
};

class User
{
private:
    int time = 0;
    unsigned timeSeed = 0;
    poisson_distribution<int> distribution;

public:
    int userId;
    vector<Task> tasks;

    // constructor
    User(int userId, double mean) : userId(userId), distribution(mean)
    {
        random_device rd;
        if (generateDifferentCodeEverytime)
            timeSeed = chrono::system_clock::now().time_since_epoch().count(); // Seed with current time
        // generator.seed(rd() + userId + timeSeed);                              // Seed the random number generator
        generator.seed(userId + timeSeed); // Seed the random number generator
        // generator.seed(userId);
    }

    // Generate a vector of tasks
    vector<Task> generateTasks(int numTasks)
    {
        for (int i = 0; i < numTasks; ++i)
        {
            time += distribution(generator);
            // Generate random execution time, user information, and relative deadline for the task
            int executionTime = 1 + (rand() % maxExecutionTime);
            int userInformation = userId;
            int relativeDeadline = 0;
            double reliability = rand() * 1.0 / (RAND_MAX);
            tasks.emplace_back(time, executionTime, userInformation, relativeDeadline, reliability);
        }
        return tasks;
    }
};

class Users
{
private:
    int numUsers;
    double arrivalRate;
    int numTasks;

public:
    Users(int numUsers, double mean, int numTasks) : numUsers(numUsers), arrivalRate(mean), numTasks(numTasks) {}

    vector<vector<Task>> generateAllUserTasks()
    {
        vector<vector<Task>> allUserTasks;
        for (int i = 1; i <= numUsers; ++i)
        {
            User user(i, arrivalRate);
            vector<Task> userTasks = user.generateTasks(numTasks);
            allUserTasks.push_back(userTasks);
        }
        return allUserTasks;
    }
};

bool compareTasksByArrivalTime(const Task &task1, const Task &task2)
{
    return task1.arrivalTime < task2.arrivalTime;
}

// bool compareTasksBySRT(const Task &task1, const Task &task2)
// {
//     return task1.relativeDeadline - task1.executionTime -task1.arrivalTime < task2.relativeDeadline - task2.executionTime -task2.arrivalTime;
// }

bool sort_by_heuristic(const pair<double, pair<int, Task *>> &task1, const pair<double, pair<int, Task *>> &task2)
{
    return task1.first < task2.first;
}

class Server
{
public:
    double total_cost = 0;     // total cost of scheduling
    vector<Task> waiting_area; // unscheduled tasks
    int num_of_machines;
    int num_of_users;
    int num_of_jobs_failed;     // number of jobs failed while executing
    int num_jobs_not_scheduled; // number of jobs which could not be scheduled
    int free_machines_size;
    int num_jobs_not_reliable;
    vector<int> jobs_failed;    // number of jobs failed on each machine
    vector<int> jobs_scheduled; // total number of jobs scheduled on each machine
    // vector<int> m_finish; //finishing time of all machines
    vector<vector<Task>> machines_scheduling_history; // for history of scheduling
    vector<double> shared_trust;
    vector<vector<double>> direct_trust;
    vector<vector<int>> num_of_tasks;            // storing no. of tasks of a user done on a machine
    vector<vector<int>> num_of_successful_tasks; // storing no. of successful tasks of a user done on a machine
    vector<double> base_cost;                    // store BR for all machines
    vector<double> probability;                  // probability of success/ failure of machine

    Server(int num_of_machines, int num_of_users)
    {
        this->num_of_machines = num_of_machines;
        this->num_of_users = num_of_users;
        // m_finish.resize(num_of_machines);
        machines_scheduling_history.resize(num_of_machines);
        shared_trust.resize(num_of_machines, 0.5);
        direct_trust.resize(num_of_machines, vector<double>(num_of_users + 1, 0.5)); // indexing of users is with 1
        num_of_tasks.resize(num_of_machines, vector<int>(num_of_users + 1, 0));
        num_of_successful_tasks.resize(num_of_machines, vector<int>(num_of_users + 1, 0));
        num_of_jobs_failed = 0;
        num_jobs_not_scheduled = 0;
        num_jobs_not_reliable = 0;
        jobs_failed.resize(num_of_machines);
        base_cost.resize(num_of_machines, 1);
        probability.resize(num_of_machines, init_failure_prob);
        jobs_scheduled.resize(num_of_machines, 0);
    }

    double cost_function(const Task &task, int machine_index)
    {
        // assuming base = 30% of base cost for now
        double cost = (task.finishingTime - task.startingTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index]));
        // cout<<"For task arriving at "<<task.arrivalTime<<" finishing at "<<task.finishingTime-task.startingTime<<" "<<task.executionTime<<" cost is "<<cost<<endl;
        return cost;
    }

    double heuristic(const Task &task, int machine_index)
    {
        double cost = 1;  // random
        if(heuristic_num == 0)
        {
            // cost /= task.arrivalTime;      // wrong measure but to check if weights are correct in gradient
            cost *= (task.executionTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index])); // cost
            cost /= ((task.relativeDeadline - task.executionTime - task.arrivalTime));                                                                                  // srt (deadline)
            cost *= (shared_trust[machine_index] - task.reliability);                                                                                                   // reliability
            return cost;
        }
        else if(heuristic_num == 1)
        {
            // cost /= task.arrivalTime;      // wrong measure but to check if weights are correct in gradient
            cost *= (task.executionTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index])); // cost                                                                                                   // reliability
            return cost;
        }
        else if(heuristic_num == 2)
        {
            // cost /= task.arrivalTime;      // wrong measure but to check if weights are correct in gradient
            cost /= ((task.relativeDeadline - task.executionTime - task.arrivalTime));                                                                                  // srt (deadline)                                                                                                 // reliability
            return cost;
        }
        else if(heuristic_num == 3)
        {
            cost *= (shared_trust[machine_index] - task.reliability);                                                                                                   // reliability
            return cost;
        }
        else //heuristic_num == 4
        {                                                                                             
            return cost;
        }
    }


    void schedule1(int time, set<int> &free_machines)
    {
        for (auto it = waiting_area.begin(); it != waiting_area.end();)
        {
            if (it->relativeDeadline < time + it->executionTime)
            {
                bool flag = 0;
                for (int machine_index = 0; machine_index < num_of_machines; machine_index++)
                {
                    if (it->reliability <= direct_trust[machine_index][it->userInformation])
                    {
                        flag = 1;
                        break;
                    }
                }
                if (flag == 0)
                {
                    num_jobs_not_reliable++;
                    // cout << "Task arriving at " << it->arrivalTime << " doesnt meet reliability requirement of any machine "  <<endl;
                }
                else
                {
                    // cout << "Task arriving at " << it->arrivalTime << " from user "
                    // << it->userInformation << " can't be scheduled." << endl;
                }
                num_jobs_not_scheduled++;
                it = waiting_area.erase(it); // Erase current task and move iterator to the next
            }
            else
            {
                ++it; // Move to the next task
            }
        }
        // sort the unscheduled tasks
        // sort(waiting_area.begin(), waiting_area.end(), compareTasksBySRT);
        vector<pair<double, pair<int, Task *>>> candidate_tasks; // {cost,task}
        for (const int &machine_index : free_machines)
        {
            for (auto it = waiting_area.begin(); it != waiting_area.end(); it++)
            {
                // cout<<it->reliability<<" "<<direct_trust[machine_index][it->userInformation]<<endl;
                if (it->reliability <= direct_trust[machine_index][it->userInformation])
                {
                    double heuristic_cost = heuristic(*it, machine_index);
                    candidate_tasks.push_back({heuristic_cost, {machine_index, &(*it)}});
                }
            }
        }
        sort(candidate_tasks.begin(), candidate_tasks.end(), sort_by_heuristic);
        vector<Task> done_task;
        set<int> done_machine;
        for (int ind = 0; ind < candidate_tasks.size(); ind++)
        {
            auto it = ((candidate_tasks[ind]).second).second;
            if (find(done_task.begin(), done_task.end(), *it) != done_task.end())
            {
                continue;
            }
            int machine_index = candidate_tasks[ind].second.first;
            if (done_machine.find(machine_index) != done_machine.end())
                continue;
            // cout << "Scheduled task arriving at " << it->arrivalTime << " from user "
            //  << it->userInformation << " to machine "
            //  << machine_index << ".\n";
            free_machines_size--;
            (*it).startingTime = time;
            // At the time of assigning I have given finishing time this can be changed if the task fails in between
            (*it).finishingTime = time + (*it).executionTime;
            num_of_tasks[machine_index][it->userInformation]++;
            jobs_scheduled[machine_index]++;
            machines_scheduling_history[machine_index].push_back(*it);
            auto it_to_erase = find(waiting_area.begin(), waiting_area.end(), *it);
            if (it_to_erase != waiting_area.end())
            {
                waiting_area.erase(it_to_erase);
            }
            done_task.push_back(*it);
            done_machine.insert(machine_index);
        }
    }
    // generate 0 with probability p and 1 with probability 1-p
    int generateRandomWithProbability(double p)
    {
        // random_device rd;
        // mt19937 gen(rd());
        bernoulli_distribution d(p);
        return d(generator) ? 0 : 1;
    }
    void server_start(vector<Task> arrivingTasks) // sorted wrt arrival time
    {
        int arrival_index = 0;
        for (int time = 0; time < MaxTime; time++)
        {
            if (time % 10000 == 0 && time != 0)
            {
                for (int i = 0; i < num_of_machines; i++)
                {
                    probability[i] = jobs_failed[i] / (jobs_scheduled[i] + 1);
                }
            }
            // putting tasks into waiting area
            while (arrival_index != arrivingTasks.size() && time >= arrivingTasks[arrival_index].arrivalTime)
            {
                arrivingTasks[arrival_index].relativeDeadline = arrivingTasks[arrival_index].arrivalTime + K;
                waiting_area.push_back(arrivingTasks[arrival_index]);
                arrival_index++;
            }

            // iterating over all the machines and checking if it has a task assigned to it, and if then probabilistically finding whether failure
            for (int machine_index = 0; machine_index < num_of_machines; machine_index++)
            {
                if (machines_scheduling_history[machine_index].size() != 0 and machines_scheduling_history[machine_index].back().finishingTime > time)
                {
                    // machine is not free
                    // generate 0 with probability p and 1 with probability 1-p
                    int result = generateRandomWithProbability(probability[machine_index]);
                    if (result == 0) // means task has failed
                    {
                        machines_scheduling_history[machine_index].back().finishingTime = time;
                        num_of_jobs_failed++;
                        jobs_failed[machine_index]++;
                        // cout<<"Task arriving at "<<machines_scheduling_history[machine_index].back().arrivalTime<<" in machine "<<machine_index<<" failed"<<endl;
                        // updating direct trust here as task got completed here
                        int user_index = machines_scheduling_history[machine_index].back().userInformation;
                        direct_trust[machine_index][user_index] = (num_of_successful_tasks[machine_index][user_index] * 1.0 / num_of_tasks[machine_index][user_index]);
                        // task has failed and its finishing time is not start time + execution time
                    }
                }
                else if (machines_scheduling_history[machine_index].size() != 0 and time == machines_scheduling_history[machine_index].back().finishingTime)
                { // checking whether task is successful, by checking whether startTime + executionTime is equal to time

                    // generate 0 with probability p and 1 with probability 1-p
                    int result = generateRandomWithProbability(probability[machine_index]);
                    if (result == 0) // means task has failed
                    {
                        machines_scheduling_history[machine_index].back().finishingTime = time;
                        num_of_jobs_failed++;
                        jobs_failed[machine_index]++;
                        // cout<<"Task arriving at "<<machines_scheduling_history[machine_index].back().arrivalTime<<" in machine "<<machine_index<<" failed"<<endl;
                        // updating direct trust here as task got completed here
                        int user_index = machines_scheduling_history[machine_index].back().userInformation;
                        direct_trust[machine_index][user_index] = (num_of_successful_tasks[machine_index][user_index] * 1.0 / num_of_tasks[machine_index][user_index]);
                        // task has failed and its finishing time is not start time + execution time
                    }

                    else
                    {
                        int user_index = machines_scheduling_history[machine_index].back().userInformation;
                        num_of_successful_tasks[machine_index][user_index]++;
                        direct_trust[machine_index][user_index] = (num_of_successful_tasks[machine_index][user_index] * 1.0 / num_of_tasks[machine_index][user_index]);
                    }
                }
            }

            // calculate shared trust
            for (int i = 0; i < num_of_machines; i++)
            {
                double temp = 0;
                for (int j = 0; j < num_of_users; j++)
                {
                    temp += direct_trust[i][j + 1];
                }
                temp /= num_of_users;
                shared_trust[i] = temp;
            }

            // Checking if any machine is free to be scheduled
            set<int> free_machines;
            for (int machine_index = 0; machine_index < num_of_machines; machine_index++)
            {
                vector<Task> current_machine_history = machines_scheduling_history[machine_index];
                if (current_machine_history.size() == 0 or current_machine_history.back().finishingTime <= time)
                {
                    free_machines.insert(machine_index);
                }
            }

            free_machines_size = free_machines.size();

            if (free_machines_size != 0)
            {
                schedule1(time, free_machines);
            }

            if (arrival_index == arrivingTasks.size() and waiting_area.size() == 0 and free_machines_size == num_of_machines)
            {
                cout << "Ending Simulation" << endl;
                for (int i = 0; i < machines_scheduling_history.size(); i++)
                {

                    for (const auto &task : machines_scheduling_history[i])
                    {
                        double cost = cost_function(task, i);
                        total_cost += cost;
                    }
                }
                break;
            }
        }
    }
    void analyze(double i,int code,int heuristic_num = 0)
    {
        double total_tasks = (numberOfUsers * numTasksPerUser);
        // cout << "Numner of Jobs failed while executing are : " << num_of_jobs_failed << endl;
        // cout << "Total Number of Jobs which failed because of deadline : " << num_jobs_not_scheduled << endl;
        // cout << "Total Number of Jobs which failed because of reliability requirements : " << num_jobs_not_reliable << endl;
        // for (int i = 0; i < num_of_machines; i++)
        // {
        //     cout << "Numner of Jobs failed on machine " << i + 1 << " is " << jobs_failed[i] << endl;
        // }
        // cout << "Total cost of execution is " << total_cost << endl;
        // cout << "Total cost per scheduled task is "<< total_cost/(numberOfUsers*numTasksPerUser -  num_jobs_not_scheduled)<<endl;
        double avg_cost = (total_cost / (total_tasks - num_jobs_not_scheduled - num_of_jobs_failed));
        double reliable_req = (num_jobs_not_reliable * 1.0) / total_tasks;
        double deadline_req = (num_jobs_not_scheduled * 1.0 - num_jobs_not_reliable) / total_tasks; // jobs failed due to deadline
        // outputFile << deadline_req << endl;
        cout<< heuristic_num<<" " << i << " " << deadline_req << endl;
        if (code==1) data.push_back({i, (double)deadline_req});
        if (code==2) data.push_back({i, (double)deadline_req});
        if (code==3) data.push_back({i, (double)num_of_jobs_failed});
        if (code==4) data.push_back({i, (double)reliable_req});
        if (code==5) data.push_back({i, (double)deadline_req});
        if (code==6) data.push_back({i, (double)deadline_req});
        if (code==7) data.push_back({i, avg_cost, reliable_req, deadline_req});
    }
};

void writeToCSV(const vector<vector<double>> &data, const std::string &filename)
{
    std::ofstream outputFile(filename, std::ios::app);

    if (!outputFile.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (const auto &pair : data)
    {
        int i=0;
        for(auto x:pair){
            if(i==pair.size() - 1)
            outputFile<<x;
            else
            outputFile << x<<",";
            i++;
        }
        outputFile<<endl;
    }

    outputFile.close();
}

int main()
{
    Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
    vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
    vector<Task> arrivingTasks;
    cout << "Arrived Tasks" << endl;

    for (const auto tasks : allUserTasks)
    {
        arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
    }

    sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);
    // number of machines vs jobs failed due to deadline
    for (int i = start; i <= stop; i += step)
    {
        numberOfMachines = i;
        outputFile << numberOfMachines << endl;
        // for (const auto task : arrivingTasks)
        // {
        //     cout << "Arrival Time: " << task.arrivalTime << ", Execution Time: " << task.executionTime
        //          << ", User Information: " << task.userInformation << ", Relative Deadline: " << task.relativeDeadline << endl;
        // }
        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,1,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,1,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,1,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,1,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,1,4);
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    start = 100;
    stop = 601;
    step = 50;
    K= 100;

    numberOfMachines = 50;
    // K vs no of jobs failed due to deadline
    for (int i = start; i <= stop; i += step)
    {
        K = i;
        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,2,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,2,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,2,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,2,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,2,4);
    }

    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    start = 1;
    stop = 52;
    step = 5;
    //failure prob vs jobs failed while executing
    for (int i = start; i <= stop; i += step)
    {
        init_failure_prob = (i*1.0)/1000;
        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,3,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,3,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,3,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,3,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,3,4);
    }

    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    start = 15;
    stop = 65;
    step = 5;
    //failure prob vs jobs failed due to reliability
    for(int i=start; i<=stop; i+= step)
    {   //will increase because shared trust will decrease.
        init_failure_prob = (i*1.0)/1000;
        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,4,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,4,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,4,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,4,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,4,4);
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    start = 100;
    stop = 1000;
    step = 100;

    init_failure_prob = 0.001;
    K = 500;
    for(int i=start; i<=stop; i+= step)
    {   //will increase because shared trust will decrease.
        numTasksPerUser = i;
        Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
        vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
        vector<Task> arrivingTasks;
        for (const auto tasks : allUserTasks)
        {
            arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
        }

        sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);

        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,5,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,5,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,5,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,5,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,5,4);
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    numTasksPerUser = 50;
    start = 15;
    stop = 80;
    step = 5;
    //failure prob vs jobs failed due to deadline
    init_failure_prob = 0.001;
    K = 500;
    for(int i=start; i<=stop; i+= step)
    {   //will increase because shared trust will decrease.
        numberOfUsers = i;
        Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
        vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
        vector<Task> arrivingTasks;
        for (const auto tasks : allUserTasks)
        {
            arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
        }

        sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);

        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,6,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,6,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,6,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,6,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,6,4);
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    numTasksPerUser = 1000;
    Users user(numberOfUsers, meanArrivalRate, numTasksPerUser);
    vector<vector<Task>> allUserTask = user.generateAllUserTasks();
    vector<Task> arrivingTask;
    for (const auto tasks : allUserTask)
    {
        arrivingTask.insert(arrivingTask.end(), tasks.begin(), tasks.end());
    }

    sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);
    K=50;
    start = 5;
    stop = 75;
    step = 5;
    init_failure_prob = 0.02;
    for (int i = start; i <= stop; i += step)
    {
        numberOfMachines = i;
        heuristic_num = 0;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,7,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,7,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,7,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,7,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,7,4);     //number of machines vs cost
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");
    data.clear();

    numberOfMachines = 50;
    numTasksPerUser = 50;
    start = 35;
    stop = 100;
    step = 5;
    //failure prob vs jobs failed due to deadline
    init_failure_prob = 0.01;
    K = 500;
    for(int i=start; i<=stop; i+= step)
    {   //will increase because shared trust will decrease.
        numberOfUsers = i;
        Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
        vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
        vector<Task> arrivingTasks;
        for (const auto tasks : allUserTasks)
        {
            arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
        }

        sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);

        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,7,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,7,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,7,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,7,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,7,4);      //number of Users vs cost
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();

    numberOfMachines = 50;
    numTasksPerUser = 50;
    numberOfUsers = 50;
    start = 35;
    stop = 100;
    step = 5;
    //failure prob vs jobs failed due to deadline
    init_failure_prob = 0.01;
    K = 500;
    for(int i=start; i<=stop; i+= step)
    {   //will increase because shared trust will decrease.
        numTasksPerUser = i;
        Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
        vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
        vector<Task> arrivingTasks;
        for (const auto tasks : allUserTasks)
        {
            arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
        }

        sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);

        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,7,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,7,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,7,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,7,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,7,4);        //number of tasks per user vs cost
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");

    data.clear();
    K=50;
    start = 50;
    stop = 500;
    step = 50;
    numberOfMachines = 10;
    init_failure_prob = 0.02;
    for (int i = start; i <= stop; i += step)
    {
        K = i;
        Server server1(numberOfMachines, numberOfUsers);
        server1.server_start(arrivingTasks);
        server1.analyze(i,7,0);

        heuristic_num = 1;
        Server server2(numberOfMachines, numberOfUsers);
        server2.server_start(arrivingTasks);
        server2.analyze(i,7,1);

        heuristic_num = 2;
        Server server3(numberOfMachines, numberOfUsers);
        server3.server_start(arrivingTasks);
        server3.analyze(i,7,2);

        heuristic_num = 3;
        Server server4(numberOfMachines, numberOfUsers);
        server4.server_start(arrivingTasks);
        server4.analyze(i,7,3);

        heuristic_num = 4;
        Server server5(numberOfMachines, numberOfUsers);
        server5.server_start(arrivingTasks);
        server5.analyze(i,7,4);      //K vs cost
    }
    data.push_back({-1,-1});
    writeToCSV(data, "output.csv");
    data.clear();

    return 0;
}