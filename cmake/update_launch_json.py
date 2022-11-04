import json
import argparse
import pathlib
import sys


# GDB config for Microsoft C/C++ plugin
class GdbConfig:
    def __init__(self, omnetpp, target, rundbg, args, cwd, debugger, setup_commands):
        self.model = {
            "name": "Launch {} - GDB (OMNeT++)".format(target),
            "type": "cppdbg",
            "request": "launch",
            "program": rundbg,
            "args": self.escapeArgs(args),
            "stopAtEntry": False,
            "cwd": cwd,
            "externalConsole": False,
            "MIMode": "gdb",
            "miDebuggerPath": debugger,
            "setupCommands": setup_commands
        }

    @property
    def name(self):
        return self.model['name']

    def escapeArgs(self, args):
        escaped_args = []
        escape_next = False
        for arg in args:
            if escape_next:
                escaped_args.append(arg.replace(';', '\\;'))
                escape_next = False
            else:
                escaped_args.append(arg)
                escape_next = (arg == '-n')
        return escaped_args


# Config for CodeLLDB extension
class CodeLldbConfig:
    def __init__(self, omnetpp, target, rundbg, args, cwd):
        self.model = {
            "name": "Launch {} - CodeLLDB (OMNeT++)".format(target),
            "type": "lldb",
            "request": "launch",
            "program": rundbg,
            "args": args,
            "stopOnEntry": False,
            "cwd": cwd,
            "initCommands": []
        }

        formatter = pathlib.Path(
            omnetpp, 'python/omnetpp/lldb/formatters/omnetpp.py')
        if formatter.is_file():
            cmd = "command script import {}".format(formatter.absolute())
            self.model['initCommands'].append(cmd)

    @property
    def name(self):
        return self.model['name']


def main():
    parser = argparse.ArgumentParser(
        description='Add OMNeT++ debug configuration to VSCode.')
    parser.add_argument(
        '--debug-config', type=str,
        help='Config to export. Valid values are GDB and CodeLLDB.')
    parser.add_argument(
        '--omnetpp-root', type=str, help="OMNeT++ root folder.")
    parser.add_argument(
        '--gdb-command', type=str, help="GDB executable.", default="gdb")
    parser.add_argument(
        '--setup-commands', type=str, required=False,
        help="JSON file with custom setup commands.")
    parser.add_argument(
        'launch_json', type=str,
        help="Location of launch.json to be updated.")

    args = parser.parse_args()

    # Path to launch.json file
    launch_file = pathlib.Path(args.launch_json)

    # Load additional commands for the debugger if file is given and exists
    setup_commands = []
    if 'setup_commands' in args:
        setup_commands_file = pathlib.Path(args.setup_commands)
        if setup_commands_file.is_file():
            try:
                with setup_commands_file.open() as f:
                    setup_commands = json.load(f)
            except ValueError as e:
                print('Invalid setup commands in file: {}'.format(e))

    configs = []
    for target_config in pathlib.Path.cwd().glob('vscode-debug/*.json'):
        with target_config.open("r") as f:
            target = json.load(f)
            target_name = target_config.stem

            config = None
            if args.debug_config == "GDB":
                config = GdbConfig(
                    args.omnetpp_root, target_name, target['exec'],
                    target['args'], target['working_directory'],
                    args.gdb_command, setup_commands)
            elif args.debug_config == "CodeLLDB":
                config = CodeLldbConfig(
                    args.omnetpp_root, target_name, target['exec'],
                    target['args'], target['working_directory'])

            if config:
                configs.append(config)


    launch_settings = {}
    if launch_file.is_file():
        with launch_file.open("r") as f:
            try:
                launch_settings = json.load(f)
            except ValueError:
                print("launch.json has invalid content, aborting.")
                sys.exit(1)

    # create empty configurations list if missing
    if 'configurations' not in launch_settings:
        launch_settings['configurations'] = []

    # keep non-generated configurations
    config_names = [config.name for config in configs]
    launch_configs = []
    for config in launch_settings['configurations']:
        if not config['name'] in config_names:
            launch_configs.append(config)

    # append generated configurations
    for config in configs:
        launch_configs.append(config.model)
    launch_settings['configurations'] = launch_configs

    with launch_file.open("w") as f:
        try:
            json.dump(launch_settings, f, indent=4)
        except ValueError as e:
            print("Aborting, updated launch.json would be invalid: ", e)
            sys.exit(1)


if __name__ == "__main__":
    main()
