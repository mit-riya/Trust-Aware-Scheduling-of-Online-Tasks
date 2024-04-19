# Trust Aware Scheduling of Online Tasks in FoG Nodes
schedules tasks in a way that minimizes cost, meets reliability requirements, and satisfies deadline constraints while considering the trustworthiness of the nodes and users. The system consists of multiple Fog nodes, each with
shared trust values and individual trust ratings from users.

---

## Run the code
Run the file named server_without_forloop.cpp
```bash
g++ -std=c++17 server_without_forloop.cpp
```
```bash
./a.out
```

Run the file named server.cpp 
```bash
g++ -std=c++17 server.cpp
```
```bash
./a.out
```

Run the file named
```bash
python Untitled.ipynb
```

## Different Files
### server.cpp
This file contains the code for the server with simulations being run on different input values and it gives different outputs corresponding to it. The output given by this file is printed in output.csv, which is further used by Untitled.ipynb to analyze results.
### server_without_forloop.cpp
This file contains the code for the server with only one instance being run on it. This file can be run by changing the input fields defined globally at the beginning of the cpp file. The output of this file is printed on the terminal itself
### Untitled.ipynb
This python notebook takes input from the CSV file and makes different graphs required to analyze different output given by different heuristics and input values
### HPC_Project.pdf
This pdf contains information about how our approach to solving this problems and different heuristics we have used.

## Author Info

#### Sahil Mehul Dhanayak

- Email - [aditya.cse21@iitg.ac.in](mailto:aditya.cse21@iitg.ac.in)
- Github - [guptaaditya30121](https://github.com/guptaaditya30121)

#### Riya Mittal

- Email - [aditya.cse21@iitg.ac.in](mailto:aditya.cse21@iitg.ac.in)
- Github - [guptaaditya30121](https://github.com/guptaaditya30121)

#### Aditya Gupta

- Email - [aditya.cse21@iitg.ac.in](mailto:aditya.cse21@iitg.ac.in)
- Github - [guptaaditya30121](https://github.com/guptaaditya30121)

