#!/usr/bin/env python3

import argparse
import sys
import os
from itertools import zip_longest
import tempfile
import definitions as defs
sys.path.insert(0, os.path.join(defs.BINARY_DIR_PREFIX, 'lib', 'netlist_paths'))
from py_netlist_paths import RunVerilator, Netlist, Waypoints, Options


DEFAULT_DOT_FILE = 'graph.dot'

def write_table(rows, fd):
    """
    Write the table rows out to fd and calculate max widths for each column.
    """
    widths = [0] * len(rows[0])
    for row in rows:
        for i, col in enumerate(row):
            widths[i] = max(widths[i], len(col))
    fmt = ' '.join('{row['+str(i)+']:<{widths['+str(i)+']}}' for i in range(len(rows[0])))
    fmt += '\n'
    fd.write(fmt.format(row=rows[0], widths=widths))
    fd.write(fmt.format(row=['-'*w for w in widths], widths=widths))
    for row in rows[1:]:
        fd.write(fmt.format(row=row, widths=widths))

def dump_names(netlist, regex, fd):
    """
    Dump a table of names and their attributes matching regex to fd.
    """
    rows = []
    HDR = ['Name', 'Type', 'DType', 'Direction', 'Location']
    rows.append(HDR)
    vertices = netlist.get_named_vertices(regex)
    vertices = sorted(vertices, key=lambda x: (x.get_name(),
                                               x.get_ast_type(),
                                               x.get_dtype_str(),
                                               x.get_direction()))
    for vertex in vertices:
        rows.append((vertex.get_name(),
                     vertex.get_ast_type(),
                     vertex.get_dtype_str(),
                     vertex.get_direction(),
                     vertex.get_location_str()))
    # Write the table out.
    write_table(rows, fd)

def dump_path_report(netlist, path, fd):
    """
    Report the details of a path. Items in the path alternate between variable
    and statement, starting and ending with a variable.
    """
    def grouper(iterable, n, fillvalue=None):
        """
        Collect data into fixed-length chunks or blocks
          grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
        From https://docs.python.org/3/library/itertools.html#recipes
        """
        args = [iter(iterable)] * n
        return zip_longest(*args, fillvalue=fillvalue)

    #rows = []
    #rows.append(('Name', 'Type', 'Statement', 'Location'))
    #rows.append((path[0].get_name(), path[0].get_dtype_str(), '', ''))
    #for stmt, var in grouper(path[1:], 2):
    #    rows.append((var.get_name(),
    #                 var.get_dtype_str(),
    #                 stmt.get_ast_type(),
    #                 stmt.get_location_str()))
    ## Write the table out.
    #write_table(rows, fd)
    for node in path:
        print("{} {}".format(node.get_ast_type(), node.get_name()));

def dump_path_list_report(netlist, paths, fd):
    """
    Report a list of paths.
    """
    for i, path in enumerate(paths):
        print('Path {}'.format(i))
        dump_path_report(netlist, path, fd)

def main():
    parser = argparse.ArgumentParser(description="Query a Verilog netlist")
    parser.add_argument('files',
                        nargs='+',
                        help='Input files')
    parser.add_argument('-c', '--compile',
                        action='store_true',
                        help='Run Verilator to compile a netlist')
    parser.add_argument('-I',
                        metavar='include_path',
                        help='Add an source include path (only with --compile)')
    parser.add_argument('-D',
                        metavar='definition',
                        help='Define a preprocessor macro (only with --compile)')
    parser.add_argument('-o', '--output',
                        default=None,
                        dest='output_file',
                        metavar='output file',
                        help='Specify an output file')
    parser.add_argument('--dump-names',
                        nargs='?',
                        default=None,
                        const='.*',
                        metavar='regex',
                        help='Dump all names, filter by regex')
    parser.add_argument('--dump-dot',
                        action='store_true',
                        help='Dump a dotfile of the netlist\'s graph')
    parser.add_argument('--from',
                        dest='start_point',
                        metavar='point',
                        help='Start point')
    parser.add_argument('--to',
                        dest='finish_point',
                        metavar='point',
                        help='Finish point')
    parser.add_argument('--through',
                        action='append',
                        default=[],
                        dest='through_points',
                        metavar='point',
                        help='Though point')
    parser.add_argument('--avoid',
                        action='append',
                        default=[],
                        dest='avoid_points',
                        metavar='point',
                        help='Avoid point')
    parser.add_argument('--all-paths',
                        action='store_true',
                        help='Find all paths between two points (exponential time)')
    parser.add_argument('--regex',
                        action='store_true',
                        help='Enable regular expression matching of names')
    parser.add_argument('-v', '--verbose',
                        action='store_true',
                        help='Print execution information')
    parser.add_argument('-d', '--debug',
                        action='store_true',
                        help='Print debugging information')
    args = parser.parse_args()

    # Setup options
    if args.regex:
        Options.get_instance().set_match_regex()
    if args.verbose:
        Options.get_instance().set_verbose()
    if args.debug:
        Options.get_instance().set_debug()

    try:

        # Verilator compilation
        # (Only supports one source file currently, useful for testing.)
        if args.compile:
            if args.output_file == None:
                output_filename = next(tempfile._get_candidate_names())
            else:
                output_filename = args.output_file
            comp = RunVerilator(defs.INSTALL_PREFIX)
            if comp.run(args.files[0], output_filename) > 0:
                raise RuntimeError('error compiling design')
        else:
            if len(args.files) != 1:
                raise RuntimeError('cannot specify multiple netlist XML files')
            output_filename = args.files[0]

        # Create the netlist
        netlist = Netlist(output_filename)

        # Delete the temporary XML output file.
        if args.compile and args.output_file == None:
            os.remove(output_filename)

        # Dump names
        if args.dump_names:
            dump_names(netlist, args.dump_names, sys.stdout)
            return 0

        # Dump graph dotfile
        if args.dump_dot:
            netlist.dump_dot_file(args.output_file if args.output_file else DEFAULT_DOT_FILE)
            return 0

        # Point-to-point path
        if args.start_point and args.finish_point:
            waypoints = Waypoints()
            waypoints.add_start_point(args.start_point)
            waypoints.add_finish_point(args.finish_point)
            [waypoints.add_through_point(point) for point in args.through_points]
            [waypoints.add_avoid_point(point) for point in args.avoid_points]
            if args.all_paths:
                path = netlist.get_all_paths(waypoints)
                dump_path_list_report(netlist, path, sys.stdout)
            else:
                path = netlist.get_any_path(waypoints)
                dump_path_report(netlist, path, sys.stdout)
            return 0

        # Fan out paths
        if args.start_point and not args.finish_point:
            if len(args.through_points) > 0:
                raise RuntimeError('cannot specify through points with fanout paths')
            if len(args.avoid_points) > 0:
                raise RuntimeError('cannot specify avoid points with fanout paths')
            paths = netlist.get_all_fanout_paths(args.start_point)
            dump_path_list_report(netlist, paths, sys.stdout)
            return 0

        # Fan in paths
        if args.finish_point and not args.start_point:
            if len(args.through_points) > 0:
                raise RuntimeError('cannot specify through points with fanin paths')
            if len(args.avoid_points) > 0:
                raise RuntimeError('cannot specify avoid points with fanin paths')
            paths = netlist.get_all_fanin_paths(args.finish_point)
            dump_path_list_report(netlist, paths, sys.stdout)
            return 0

    except RuntimeError as e:
        print('Error: '+str(e))
        return 1


if __name__ == '__main__':
    sys.exit(main())
