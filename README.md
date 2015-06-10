# Highway Flocking

This is the source code accompanying the master thesis from Sindre Johansen ([@sindreij](https://github.com/sindreij)) and Andreas Løvland (@[AndrLov](https://github.com/andrlov)) spring of 2015. This repository should contain everything needed to run the simulation. However, no warranties are given. The following code might work, or it might crash your machine (probably both). The code is given away under apache license for inspiration, but this is not a complete product.

For an overview over the project, take a look at https://highwayflocking.github.io/.

PS: Note that the textures in this project is a bit different from the one presented in the video and the thesis. We needed to change the textures because of licensing reasons.

## About the Source Code

This code consists of two components, the Highway Simulator, and the Controller. The Simulator is written in C++, using Unreal Engine. The Controller is written in python. To test the Simulator, you should probably download the [release](https://highwayflocking.github.io/) with pre-compiled code, the rest of this documentation is about compiling from source. The following is a list of the top folders and their content.

* **Source/** All the C++ source code for the simulator.
* **Content/** All the assets for the simulator.
* **blender/** The blender files for the assets created by us.
* **controller/** The Controller code.
* **Config/** Configuration of the simulator. Used by Unreal Editor.
* **Build/** Some build files for the simulator, for now just the icon.
* **NPRA\_Data\_Plot/** Code to plot data from NPRA. Data not included.
* **README.md** - The file you are reading
* **HighwayFlocking.uproject** - The Unreal Project.
* **gui_setup.iss** - Inno Setup project file

## Installing the requirements

First and foremost, this simulator is written using Windows. It could probably run on Linux and Mac OSX with some tweaking, however it is only tested using Windows, and a few parts of the code is written using windows only SDKs (mainly the replay runner). However, getting it to run on other OSes should only be a few lines of code (pull requests are welcome).

The main requirement is Unreal Engine. You need to register at https://www.unrealengine.com/ to download it, and it is free for non-commercial use. Installing should be simple, and this documentation assumes that you have Unreal Engine working.

Since we are using C++ code, you also need to have a source editor and compiler compatible with Unreal Engine. On Windows, this is Visual Studio.

For the controller to work, you need to have Python 2.7 installed and correctly set up. You also need to install a few requirements, *controller/requirements.txt* contains a list that should include all requirements needed. To install them, something like the following might work. However, installing matplotlib and numpy might be impossible using the following method, depending on your setup. Installation files for both numpy and matplotlib should be acquirable on the internet.

    pip install -r controller/requirements.txt

## Compiling and Running the simulator

First, you need to create the Visual Studio project. To do this, right click *HighwayFlocking.uproject*, and choose *Generate Visual Studio project files*.

Start Unreal Editor by doubleclicking *HighwayFlocking.uproject*. You will probably get a messsage saying that *UE4Editor-HighwayFlocking.dll* is missing. Click **Yes** to compile the project. When this is done, you should be able to run the project in Unreal Editor. To view the source code in Visual Studio, press *File | Open Visual Studio*.

When the simulation have started, try clicking *O* to start the spawners. Press *ESC* to close the simulator. When running the simulator, there are lots of keyboard shortcuts that can be used. These are the most important:

* Always:
 * **N** - Toggle road markings.
 * **F4** - Toggle HUD.
 * **ESC** - Exit the simulator.
 * **Left Click** - Follow a vehicle.
* As spectator:
 * **W** - Move forward.
 * **S** - Move backward.
 * **A** - Move Left.
 * **D** - Move Right.
 * **O** - Toggle Spawners.
 * **T** - Teleport.
 * **Shift + Mouse Wheel** - Zoom.
* Following a flocking vehicle:
 * **1** - Return to spectator.
 * **Mouse Wheel** - Change distance to vehicle.

When the simulator is running in Unreal Editor, you need to package the project for use with the controller. In Unreal Editor, press *File | Package Project | Windows | Windows (64 bit)* (if you cannot choose 64 bit, make sure you are using the development configuration, or choose 32 bit windows. Note that the 32 bit windows can not play replays larger than 2 GB). Choose the folder *controller/* in the project.

## Running the controller

When the simulator is packaged, you can run the controller. Make sure that all the required dependencies is installed (see a previous section). The controller consist of four top level files that can be run, *gui.py*, *remote.py*, *worker.py*, and *server.py*. *gui.py* is the simplest one, and is a simple GUI where you can try the simulator using the different configs. This is the recommended way of checking if the simulator is working. To start the gui, just doubleclick *gui.py*.

### A Distributed System

The three files *remote.py*, *worker.py* and *server.py* are three parts of a distributed system to run experiments. *worker.py* is the worker, which gets commands from a server running *server.py*. *remote.py* communicates with the server, giving it commands. *remote.py* can also be used for tasks not requring communications with the server. For an overview over what it can do, run.

    python remote.py --help

The system is configured in config.py, to create a local override, create the file local_config.py, use local_config.py.dist as a template.

Before the server can be run, you need to create the required tables. You need to have postgres setup correctly, at least version 9.4. Make sure that the config variable *DB_CONNECTION_STRING* is correct. Run the following:

    python server.py create_tables

The server is just a plain HTTP server using flask. It can be deployed as a wsgi application, or run locally using

    python server.py run

#### The worker

To deploy a worker, copy the controller folder (including the packaged simulator) to the worker machine (or use the same machine where the server is running). Make sure that the config variable *SERVER_URL* is pointing to the server. Doubleclick *worker.py* to start the worker.

#### The remote

Make sure that *SERVER_URL* is correct. When everything is running, you should be able to run the following:

    $ python remote.py workers
	2015-06-08 18:15:08,243 - INFO - _new_conn - Starting new HTTP connection (1): its-015-06.idi.ntnu.no
	 NAME       VERSION  TIME SINCE SEEN  DOING
	 ========== ======== ===============  ===============
	 worker1    0.2.1     00:00:03         get_task
	 worker2    0.2.1     00:00:04         get_task
	 worker3    0.2.1     00:00:03         get_task

This should give a list of all the workers you have deployed. To add some tasks to the queue, run for example following.

    $ python remote.py add_tasks ONCOMING_ONRAMP_BUS 4000 15000 400
	2015-06-08 18:13:29,569 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(4000)
	2015-06-08 18:13:29,569 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(4400)
	2015-06-08 18:13:29,569 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(4800)
	2015-06-08 18:13:29,569 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(5200)
	...
	2015-06-08 18:13:29,576 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(14000)
	2015-06-08 18:13:29,578 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(14400)
	2015-06-08 18:13:29,578 - INFO - add_tasks - Adding tasks oncoming_onramp_bus+througput(14800)
	2015-06-08 18:13:29,862 - INFO - _new_conn - Starting new HTTP connection (1): its-015-06.idi.ntnu.no

This will add the same tasks that were repeated 30 times for each configuration in the thesis. The first number is that starting throughput, the second number is the stop throughput, and the third is the step. This will create a tasks for each throughput from 4000 to, but not including 15000, with a step of 400 between every throughput.

The workers should now start working on the tasks. To see the uncomplete tasks, run the following:

	$ remote.py tasks
	2015-06-08 18:19:06,039 - INFO - _new_conn - Starting new HTTP connection (1): its-015-06.idi.ntnu.no
	ID   REQUEST_TIME     STARTED_TIME     STATUS_TIME      %% DONE COMPLETED WORKER     CONFIG_NAME
	==== ================ ================ ================ ======= ========= ========== ==========================
	12024 21.05.2015 15:36 Not started      21.05.2015 17:35   0.0 % False     None       symetric+througput(10000)
	12025 21.05.2015 15:36 Not started      25.05.2015 12:48   0.0 % False     None       symetric+througput(12000)
	12294 08.06.2015 18:13 08.06.2015 18:17 08.06.2015 18:18  16.3 % False     worker1    oncoming_onramp_bus+througput(4000)
	12296 08.06.2015 18:13 08.06.2015 18:17 08.06.2015 18:18   9.8 % False     worker2    oncoming_onramp_bus+througput(4800)
	12297 08.06.2015 18:13 08.06.2015 18:18 08.06.2015 18:18   3.3 % False     worker3    oncoming_onramp_bus+througput(5200)
	12298 08.06.2015 18:13 Not started      08.06.2015 18:13   0.0 % False     None       oncoming_onramp_bus+througput(5600)
	12299 08.06.2015 18:13 Not started      08.06.2015 18:13   0.0 % False     None       oncoming_onramp_bus+througput(6000)
	...

Note that only uncomplete tasks are shown. You can see which tasks the workers are working at, and how many percent done they are. To see a list of the complete results, run: (note how I specify the worker-version I want results from).

	$ remote.py results -v 0.2.1
	2015-06-08 18:20:49,336 - INFO - _new_conn - Starting new HTTP connection (1): its-015-06.idi.ntnu.no
	ID  TASKID VERSION COMPLETED_TIME   WORKER     CONFIG_NAME                                        INCIDENTS THROUGPUT
	=== ====== ======= ================ ========== ================================================== ========= =========
	9668  12061 0.2.1   21.05.2015 16:33 its-015-03 oncoming_onramp_bus+througput(5200)                        0      5150
	9667  12049 0.2.1   21.05.2015 16:32 its-015-15 oncoming_onramp_bus+througput(10800)                       0     10078
	9666  12060 0.2.1   21.05.2015 16:31 its-015-20 oncoming_onramp_bus+througput(4800)                        0      4772
	9670  12051 0.2.1   21.05.2015 16:34 its-015-22 oncoming_onramp_bus+througput(11600)                       4     10704
	9671  12052 0.2.1   21.05.2015 16:35 its-015-09 oncoming_onramp_bus+througput(12000)                       0     11018
	9669  12050 0.2.1   21.05.2015 16:33 its-015-26 oncoming_onramp_bus+througput(11200)                       0     10376
	9673  12063 0.2.1   21.05.2015 16:37 its-015-04 oncoming_onramp_bus+througput(6000)                        0      5917
	9674  12064 0.2.1   21.05.2015 16:38 its-015-08 oncoming_onramp_bus+througput(6400)                        0      6229
	9672  12062 0.2.1   21.05.2015 16:36 its-015-16 oncoming_onramp_bus+througput(5600)                        0      5519
	9677  12066 0.2.1   21.05.2015 16:40 its-015-12 oncoming_onramp_bus+througput(7200)                        0      6990
	9676  12055 0.2.1   21.05.2015 16:39 its-015-18 oncoming_onramp_bus+througput(13200)                       6     11876

Note how the number of incidents and the average measured throughput is reported. You can easily get a plot of all runs, you need to specify the configuration you want a plot of, and the workerversion.

     $ remote.py plot oncoming_onramp_bus tp -v 0.2.1

To get a plot using R (the plots used in the thesis), you need to append **-r**. You need to have R installed for this to work.

To get a more thorough look at a single run, you can look at its replay. Take for instance the 5200-throughput run above, note that this has a resultID of 9668. To play the replay, run.

	$ remote.py play_replay 9668
	2015-06-08 18:26:35,640 - INFO - _new_conn - Starting new HTTP connection (1): its-015-06.idi.ntnu.no
	2015-06-08 18:26:54,084 - INFO - download_replay - GUnzipping the replay

This will first download the replay from the server, and then unzip it. This takes some time, however, the replay is cached so that running the command a second time is much faster. The replay will start. The following keyboard shortcuts can be used to control the replay.

* **Space** - Pause / Unpause the replay
* **Arrow Left** - Play Reversed (Paused) / Slow down time (Running forwards) / Accelerate time (Running backwards)
* **Arrow Right** - Play (Paused) / Accelerate time (Running forwards) / Slow down time (Running backwards)
* **Arrow Up** - Go to next accident (slow, and bug-prone, use caution)
* **Arrow Down** - Go to previous accident (slow, and bug-prone, use caution)
* **F3** - Show behavior arrows 
* **F5** - Show/Hide HUD.
* **F6** - Show vehicle IDs (useful for movie recording)


### Remote.py subcommands

The remote command have a lot of options, this is a short guide. To get a list of options, run:

    remote.py help

#### add_task
Add one task.

Example:

    python remote.py add_task ONCOMING_ONRAMP_BUS --throughput 5000

#### add_tasks
Add multiple tasks

Example:

    python remote.py add_tasks ONCOMING_ONRAMP_BUS 4000 15000 400 -n 2

#### tasks
Show non-completed or all tasks.

#### results
Show results

#### result
Show data for a result

#### workers
Show all workers

#### cleanup
Remove tasks where the worker have abanded the task

#### plot
Plot results

#### play_replay
Play a replay from a result

#### play_file
Play a replay from a local file (does not connect to the server)

#### reset_tasks
Set the task to not started (and lets a new worker try the task)

#### movie
Create a movie from a replay, multiple commands to specify the camera to use.

#### run_locally
Runs the same task as *add_task*, but instead of having a worker run the task, the task is run locally. The results are not saved to the server. Useful for testing.

#### engine_record
Start the engine in game modus, and record to the replay specified. Useful to create a replay from a specific scenario.
