#include <bits/stdc++.h>
#include <thread>
#include <fstream>
#include <iostream>
using namespace std;
typedef long long llint;

// Mean arrival rate of tasks
#define meanArrivalRate 5.0

// Number of users
int numberOfUsers = 10;

// Maximum execution time for tasks
#define maxExecutionTime 50

// Value of K
int K = 500;

// Number of tasks per user
int numTasksPerUser = 100;

// Flag to generate different code every time
#define generateDifferentCodeEverytime 0 // 0: Same data in every run 1: Different data in every run

// Maximum time for simulation
#define MaxTime 1e8

// Initial failure probability
double init_failure_prob = 0.001;

// Heuristic number
int heuristic_num = 0;

// Number of machines
int numberOfMachines = 10;

// Random number generator
default_random_engine generator;

// Output file
::ofstream outputFile("output.txt");

// Start, stop, and step values for simulation
int start = 5;
int stop = 55;
int step = 5;

// Reset probability
int reset_prob = 0;

// Data vector
vector<vector<double>> data;

// Task class
class Task
{
public:
    int arrivalTime;       // Arrival time of the task
    int executionTime;     // Execution time of the task
    int startingTime = -1; // Starting time of a task
    int finishingTime;     // Finishing time of a task
    int userInformation;   // User information associated with the task
    int relativeDeadline;  // Relative deadline of the task
    double reliability;    // Reliability of the task

    // Constructor
    Task(int arr, int exec, int user, int deadline, double reliability) : arrivalTime(arr), executionTime(exec), userInformation(user), relativeDeadline(deadline), reliability(reliability) {}

    // Overloading the equality operator
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

// User class
class User
{
private:
    int time = 0;
    unsigned timeSeed = 0;
    poisson_distribution<int> distribution;

public:
    int userId;
    vector<Task> tasks;

    // Constructor
    User(int userId, double mean) : userId(userId), distribution(mean)
    {
        random_device rd;
        if (generateDifferentCodeEverytime)
            timeSeed = chrono::system_clock::now().time_since_epoch().count(); // Seed with current time
        generator.seed(userId + timeSeed); // Seed the random number generator
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

// Users class
class Users
{
private:
    int numUsers;
    double arrivalRate;
    int numTasks;

public:
    Users(int numUsers, double mean, int numTasks) : numUsers(numUsers), arrivalRate(mean), numTasks(numTasks) {}

    // Generate tasks for all users
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

// Compare tasks by arrival time
bool compareTasksByArrivalTime(const Task &task1, const Task &task2)
{
    return task1.arrivalTime < task2.arrivalTime;
}

// Sort tasks by heuristic value
bool sort_by_heuristic(const pair<double, pair<int, Task *>> &task1, const pair<double, pair<int, Task *>> &task2)
{
    return task1.first < task2.first;
}

// Server class
class Server
{
public:
    double total_cost = 0;     // Total cost of scheduling
    vector<Task> waiting_area; // Unscheduled tasks
    int num_of_machines;       // Number of machines
    int num_of_users;          // Number of users
    int num_of_jobs_failed;    // Number of jobs failed while executing
    int num_jobs_not_scheduled; // Number of jobs which could not be scheduled
    int free_machines_size;     // Number of free machines
    int num_jobs_not_reliable;  // Number of jobs which failed due to reliability requirements
    vector<int> jobs_failed;    // Number of jobs failed on each machine
    vector<int> jobs_scheduled; // Total number of jobs scheduled on each machine
    vector<vector<Task>> machines_scheduling_history; // History of scheduling
    vector<double> shared_trust;                      // Shared trust values
    vector<vector<double>> direct_trust;              // Direct trust values
    vector<vector<int>> num_of_tasks;                 // Number of tasks done by each user on each machine
    vector<vector<int>> num_of_successful_tasks;      // Number of successful tasks done by each user on each machine
    vector<double> base_cost;                         // Base cost for all machines
    vector<double> probability;                       // Probability of success/failure of machine

    // Constructor
    Server(int num_of_machines, int num_of_users)
    {
        this->num_of_machines = num_of_machines;
        this->num_of_users = num_of_users;
        machines_scheduling_history.resize(num_of_machines);
        shared_trust.resize(num_of_machines, 0.5);
        direct_trust.resize(num_of_machines, vector<double>(num_of_users + 1, 0.5)); // Indexing of users starts from 1
        num_of_tasks.resize(num_of_machines, vector<int>(num_of_users + 1, 0));
        num_of_successful_tasks.resize(num_of_machines, vector<int>(num_of_users + 1, 0));
        num_of_jobs_failed = 0;
        num_jobs_not_scheduled = 0;
        num_jobs_not_reliable = 0;
        jobs_failed.resize(num_of_machines);
        base_cost.resize(num_of_machines);
        ::mt19937 gen(0); // Fixed seed for the random number generator
        ::uniform_real_distribution<double> dis(1, 50);
        for (int i = 0; i < num_of_machines; i++)
        {
            base_cost[i] = dis(gen);
        }
        probability.resize(num_of_machines, init_failure_prob);
        jobs_scheduled.resize(num_of_machines, 0);
    }

    // Cost function for a task on a machine
    double cost_function(const Task &task, int machine_index)
    {
        double cost = (task.finishingTime - task.startingTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index]));
        return cost;
    }

    // Heuristic function for a task on a machine
    double heuristic(const Task &task, int machine_index)
    {
        double cost = 1; // Random value
        if (heuristic_num == 0)
        {
            cost *= (task.executionTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index])); // Cost
            cost /= ((task.relativeDeadline - task.executionTime - task.arrivalTime));                                                                                  // SRT (deadline)
            cost *= (direct_trust[machine_index][task.userInformation] - task.reliability);                                                                             // Reliability
            return cost;
        }
        else if (heuristic_num == 1)
        {
            cost *= (task.executionTime) * ((0.3) * base_cost[machine_index] + (base_cost[machine_index] * shared_trust[machine_index] * shared_trust[machine_index])); // Reliability
            return cost;
        }
        else if (heuristic_num == 2)
        {
            cost /= ((task.relativeDeadline - task.executionTime - task.arrivalTime)) * shared_trust[machine_index]; // SRT (deadline)
            return cost;
        }
        else if (heuristic_num == 3)
        {
            cost *= (direct_trust[machine_index][task.userInformation] - task.reliability); // Reliability
            return cost;
        }
        else
        {
            // Random value
            return cost;
        }
    }

    // Schedule tasks
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
                }
                else
                {
                    // Task cannot be scheduled
                }
                num_jobs_not_scheduled++;
                it = waiting_area.erase(it); // Erase current task and move iterator to the next
            }
            else
            {
                ++it; // Move to the next task
            }
        }

        // Sort the unscheduled tasks
        vector<pair<double, pair<int, Task *>>> candidate_tasks; // {cost,task}
        for (const int &machine_index : free_machines)
        {
            for (auto it = waiting_area.begin(); it != waiting_area.end(); it++)
            {
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
            free_machines_size--;
            (*it).startingTime = time;
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

    // Start the server
    void server_start(vector<Task> arrivingTasks)
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

            // Put tasks into waiting area
            while (arrival_index != arrivingTasks.size() && time >= arrivingTasks[arrival_index].arrivalTime)
            {
                arrivingTasks[arrival_index].relativeDeadline = arrivingTasks[arrival_index].arrivalTime + K;
                waiting_area.push_back(arrivingTasks[arrival_index]);
                arrival_index++;
            }

            // Iterate over all the machines and check if a task has failed or completed
            for (int machine_index = 0; machine_index < num_of_machines; machine_index++)
            {
                if (machines_scheduling_history[machine_index].size() != 0 and machines_scheduling_history[machine_index].back().finishingTime > time)
                {
                    // Machine is not free
                    vector<bool> p(100, 1); // Generate 0 with probability p and 1 with probability 1-p
                    int rand_index = 5;
                    for (int i = 0; i < probability[machine_index] * 100; i++)
                    {
                        p[rand_index] = 0;
                        rand_index = rand_index * 1000009 + 435321;
                        rand_index %= 1000081;
                        rand_index %= 100;
                    }
                    int result = p[time % 100];
                    if (result == 0) // Task has failed
                    {
                        machines_scheduling_history[machine_index].back().finishingTime = time;
                        double cost = cost_function(machines_scheduling_history[machine_index].back(), machine_index);
                        total_cost += cost;
                        num_of_jobs_failed++;
                        jobs_failed[machine_index]++;
                    }
                }
                else if (machines_scheduling_history[machine_index].size() != 0 and time == machines_scheduling_history[machine_index].back().finishingTime)
                {
                    vector<bool> p(100, 1); // Generate 0 with probability p and 1 with probability 1-p
                    int rand_index = 5;
                    for (int i = 0; i < probability[machine_index] * 100; i++)
                    {
                        p[rand_index] = 0;
                        rand_index = rand_index * 1000009 + 435321;
                        rand_index %= 1000081;
                        rand_index %= 100;
                    }
                    int result = p[time % 100];
                    if (result == 0) // Task has failed
                    {
                        machines_scheduling_history[machine_index].back().finishingTime = time;
                        double cost = cost_function(machines_scheduling_history[machine_index].back(), machine_index);
                        total_cost += cost;
                        num_of_jobs_failed++;
                        jobs_failed[machine_index]++;
                    }
                    else
                    {
                        int user_index = machines_scheduling_history[machine_index].back().userInformation;
                        double cost = cost_function(machines_scheduling_history[machine_index].back(), machine_index);
                        total_cost += cost;
                        num_of_successful_tasks[machine_index][user_index]++;
                        direct_trust[machine_index][user_index] = ((50 + num_of_successful_tasks[machine_index][user_index] * 1.0) / (100 + num_of_tasks[machine_index][user_index]));
                    }
                }
            }

            // Calculate shared trust
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

            // Check if any machine is free to be scheduled
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

    // Analyze the results
    void analyze()
    {
        double total_tasks = (numberOfUsers * numTasksPerUser);
        cout << "Number of jobs failed while executing: " << num_of_jobs_failed << endl;
        cout << "Total number of jobs which failed because of deadline: " << num_jobs_not_scheduled << endl;
        cout << "Total number of jobs which failed because of reliability requirements: " << num_jobs_not_reliable << endl;
        for (int i = 0; i < num_of_machines; i++)
        {
            cout << "Number of jobs failed on machine " << i + 1 << ": " << jobs_failed[i] << endl;
        }
        cout << "Total cost of execution: " << total_cost << endl;
        cout << "Total cost per scheduled task: " << total_cost / (numberOfUsers * numTasksPerUser - num_jobs_not_scheduled) << endl;
        double avg_cost = (total_cost / (total_tasks - num_jobs_not_scheduled - num_of_jobs_failed));
        double reliable_req = (num_jobs_not_reliable * 1.0) / total_tasks;
        double deadline_req = (num_jobs_not_scheduled * 1.0 - num_jobs_not_reliable) / total_tasks; // Jobs failed due to deadline
        cout << "Average cost: " << avg_cost << endl;
        cout << "Reliability requirement: " << reliable_req << endl;
        cout << "Deadline requirement: " << deadline_req << endl;
    }
};

int main()
{
    // Generate tasks for all users
    Users users(numberOfUsers, meanArrivalRate, numTasksPerUser);
    vector<vector<Task>> allUserTasks = users.generateAllUserTasks();
    vector<Task> arrivingTasks;

    // Flatten the 2D vector into a 1D vector
    for (const auto tasks : allUserTasks)
    {
        arrivingTasks.insert(arrivingTasks.end(), tasks.begin(), tasks.end());
    }

    // Sort tasks by arrival time
    sort(arrivingTasks.begin(), arrivingTasks.end(), compareTasksByArrivalTime);

    // Create an instance of the Server class
    Server server(numberOfMachines, numberOfUsers);

    // Start the server
    server.server_start(arrivingTasks);

    // Analyze the results
    server.analyze();

    return 0;
}
