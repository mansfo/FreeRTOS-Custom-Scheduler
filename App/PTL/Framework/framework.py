import json
import subprocess
import os
from math import ceil

def creation_config(global_conf,tasks):
    c_code = []
    c_code.append('#include "ptl_config.h"')
    c_code.append('#include <stdint.h>')
    c_code.append("SchedulerConfig_t cfg = {")
    c_code.append(f"    .xGlobalPolicy = {global_conf.get('xGlobalPolicy','NO_POLICY')},")
    var="true" if global_conf.get('xTraceEnabled','false') else "false"
    c_code.append(f"    .xTraceEnabled = {var},")
    c_code.append(f"    .uxMaxTasks = {global_conf.get('uxMaxTasks',20)},")
    c_code.append("     .xTasks ={")
    for task in tasks:
        c_code.append('{')
        pcName = task.get('pcName','')
        c_code.append(f'     .pcName = "{pcName}",')
        xEntry = task.get('xEntry','NULL')
        c_code.append(f"     .xEntry = {xEntry},")
        pvArg = task.get('pvArg','NULL')
        c_code.append(f"     .pvArg = (void*){pvArg},")
        ss = task.get('ulStackSize','configMINIMAL_STACK_SIZE*2')
        c_code.append(f"     .ulStackSize = {ss},")
        uxPriority = task.get('uxPriority','tskIDLE_PRIORITY')
        c_code.append(f"     .uxPriority = {uxPriority},")
        ulPeriod = task.get('ulPeriod',100)
        c_code.append(f"     .ulPeriod = {ulPeriod},")
        ulDeadline = task.get('ulDeadline',ulPeriod)
        c_code.append(f"     .ulDeadline = {ulDeadline},")
        xPolicy = task.get('xPolicy','NO_POLICY')
        c_code.append(f"    .xPolicy = {xPolicy},")
        ulStartOffset = task.get('ulStartOffset', '0')
        c_code.append(f"    .ulStartOffset = {ulStartOffset}")
        c_code.append('},')
    c_code.append('},')
    c_code.append(f"    .uxNumTasks = {len(tasks)}")
    c_code.append('};')
    return c_code


def read_json(filepath):
    try:
        with open(filepath,'r') as f:
            data = json.load(f)
            tasks = data.get('xTasks',[])
            global_conf = data.get('global',[])
            if not global_conf:
                print("Global configuration is empty, it will be filled with standard values.")
                global_conf = {
                    "xGlobalPolicy" : "NO_POLICY",
                    "xTraceEnabled" : "true",
                    "uxMaxTasks" : 8
                }
    except FileNotFoundError:
        print(f"ERROR: File not found at path: {filepath}")
        return [], {}
    except Exception as e:
        print(f"ERROR: Exception {e}.")
        return [],{}
    return tasks,global_conf

def write_code(code):
    import sys
    script_path_absolute = os.path.abspath(sys.argv[0])
    script_directory = os.path.dirname(script_path_absolute)
    out = "Utils/ptl_config.c"
    output_file = os.path.join(os.path.normpath(os.path.join(script_directory,"../")),out)
    try:
        with open(output_file, 'w') as f:
            f.write('\n'.join(code))
    except IOError as e:
        print(f"ERROR: is not possibol to write {out} cause {e}.") 
    
def policy_menu():
    menu_pol = []
    menu_pol.append("POLICY_SKIP")
    menu_pol.append("POLICY_KILL")
    menu_pol.append("POLICY_CATCH_UP")
    menu_pol.append("NO_POLICY")
    for (i,men) in enumerate(menu_pol):
        print(f"{i+1}) {men}")
    while True:
        try:
            policy = int(input("Make your choice: "))
            break
        except Exception as e:
            print(e)
    index = (policy) if policy >=1 and policy <=4 else len(menu_pol)
    return menu_pol[index-1]
    
def entry_menu():
    menu_task = []
    menu_task.append("vDelayRoutine")
    menu_task.append("vBusyWaitTask")
    for (i,men) in enumerate(menu_task):
        print(f"{i+1}) {men}")
    while True:
        try:
            entry = int(input("Make your choice: "))
            break
        except Exception as e:
            print(e)
    index = (entry) if entry >=1 and entry <=len(menu_task) else len(menu_task)
    return menu_task[index-1]

def check_schedulability(task_list):
    U = sum(t['pvArg'] / t['ulPeriod'] for t in task_list)
    if U > 1:
        print("CPU overload")
        return False

    tasks_sorted = sorted(task_list, key=lambda t: t['uxPriority'], reverse=True)
    for i, task in enumerate(tasks_sorted):
        C_i = task['pvArg']
        D_i = task['ulDeadline']
        R = C_i

        while True:
            interference = 0

            for hp in tasks_sorted[:i]:
                C_j = hp['pvArg']
                T_j = hp['ulPeriod']
                interference += ceil(R / T_j) * C_j

            R_next = C_i + interference

            if R_next > D_i:
                print("Deadline miss:", task['pcName'])
                return False

            if R_next == R:
                break

            R = R_next

    print("The tasks are schedulable!")
    return True


def creation_tasks():
    data = dict()
    global_conf = dict()
    print("GLOBAL CONF: ")
    global_conf['xGlobalPolicy'] = policy_menu()
    trace = input("Trace enabled? (Y/N) ")
    trace = True if trace == 'Y' or trace == 'y' else False
    global_conf['xTraceEnabled'] = trace
    max_tasks = -1
    while max_tasks > 20 or max_tasks <= 0:
        max_tasks = int(input("Max tasks: "))
    global_conf['uxMaxTasks'] = max_tasks
    n_tasks = -1
    while n_tasks > max_tasks or n_tasks <= 0:
        n_tasks = int(input("How many tasks: "))
    global_conf['uxNumTasks'] = n_tasks
    data['global'] = global_conf
    tasks = []
    for i in range(n_tasks):
        task = dict()
        print(f"{i+1})")
        name = input("Name of task: ")
        task["pcName"] = name
        task["xEntry"] = entry_menu()
        task["ulStackSize"] = "configMINIMAL_STACK_SIZE * 2"
        while True:
            try:
                priority = int(input("Priority of task : "))
                task["uxPriority"] = priority
                break
            except Exception as e:
                print(e)
        while True:
            try:
                wcet = int(input("WCET of the task : "))
                task["pvArg"] = wcet
                break
            except Exception as e:
                print(e)
        while True:
            try:
                ulPeriod = int(input("Period of task : "))
                task["ulPeriod"] = ulPeriod
                break
            except Exception as e:
                print(e)
    
        try:
            deadline = int(input("Deadline of task : "))
        except :
            deadline = ulPeriod
        task["ulDeadline"] = deadline
        choice = input('Personal policy? (Y/N): ')
        if choice == 'Y' or choice == 'y':
            policy_task = policy_menu()
        else:
            policy_task = 'NO_POLICY'
        task["xPolicy"] = policy_task
        offset_choice = input("Start offset of the task? (Y/N): ")
        while offset_choice.lower() not in {"y", "n"}:
            offset_choice = input("Start offset of the task? (Y/N): ")
        offset = 0
        if offset_choice.lower() == "y":
            offset = -1
            while offset < 0:
                try:
                    offset = int(input("Start offset: "))
                except: pass
        task["ulStartOffset"] = offset
        tasks.append(task)
    data['xTasks'] = tasks
    filepath = "./PTL/Framework/custom_scheduler.json"
    with open(filepath, "w") as file:
        json.dump(data, file,indent=4,sort_keys=True)

default_scheduler = "./PTL/Framework/test.json"

def menu():
    choosed_file = default_scheduler
    menu = {}
    menu['1']="Create tasks." 
    menu['2']="Load test."
    menu['3']="Check Schedulability"
    menu['4']="Create and run FreeRTOS skeleton."
    menu['5']="Exit"
    while True: 
        for entry in sorted(menu.keys()): 
            print(f"{entry}) {menu[entry]}")
        ans = input("Make a choice: ")
        match ans:
            case "1":
                creation_tasks()
            case "2":
                try :
                    files = subprocess.run(["ls" ,"./PTL/Framework"],capture_output= True,text= True,check= True).stdout.splitlines()
                    files = [f for f in files if f.endswith('.json')]
                except Exception:
                    print("Error")
                for (i,filename) in enumerate(files):
                    print(f"{i+1}) {filename}")
                try:
                    j = int(input("Choose the file to run: "))-1
                    choosed_file = "./PTL/Framework/"
                    if j < 0 or j>=len(files):
                        print("Not valid choice.")
                    choosed_file += files[j]
                    print(choosed_file)
                except Exception as e:
                    print(e)
            case "3": 
                tasks, _ = read_json(choosed_file)
                if not tasks:
                    print("First load the test")
                else:
                    check_schedulability(tasks)
                continue
            case "4":
                (tasks, global_conf) = read_json(choosed_file)
                if not tasks and not global_conf:
                    print("First load the test")
                    continue
                code = creation_config(global_conf,tasks)
                write_code(code)
                subprocess.run(["make"]) 
                subprocess.run(["make","qemu_start"]) 
            case "5":
                print("Bye!")
                return

def main():
    menu()
    

if __name__ == "__main__":
    main()