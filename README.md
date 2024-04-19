# Trust Aware Scheduling of Online Tasks in FoG Nodes
schedules tasks in a way that minimizes cost, meets reliability requirements, and satisfies deadline constraints while considering the trustworthiness of the nodes and users. The system consists of multiple Fog nodes, each with
shared trust values and individual trust ratings from users.

---

## Run the code
Run the file named server_without_forloop.cpp to get the single instance running with fixed values.
```bash
g++ -std=c++17 server_without_forloop.cpp
```
```bash
./a.out
```

Run the file named server.cpp to get all of the results used to plot the graphs
```bash
g++ -std=c++17 server.cpp
```
