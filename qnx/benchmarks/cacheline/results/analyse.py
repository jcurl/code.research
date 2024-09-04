#!/usr/bin/env python3

'''Analyse the results of a JSON file recorded using google-benchmark and print
the results'''

import json
import os
import re
import sys

SIZE = 256 * 1024 * 1024

def main(argv: "Sequence | None" = None) -> int:
    '''Read the json file and convert to a table'''
    if len(argv) < 1 or len(argv) > 2:
        raise Exception("Provide the .json file as the only argument")

    actual_size = SIZE
    if len(argv) == 2:
        actual_size = int(argv[1]) * 1024 * 1024

    config_file_name = argv[0]
    if not os.path.isfile(config_file_name):
        raise Exception(f"File '{config_file_name}' not found")

    with open(config_file_name, encoding="utf-8") as config_file:
        results = json.load(config_file)
    # print(json.dumps(results))

    for benchmark in results["benchmarks"]:
        # We get the stride from the name "BM_CopyStride/16"
        stride_match = re.search("BM_CopyStride/(\d+)", benchmark["name"])
        if stride_match is None:
            continue
        stride = int(stride_match.group(1))

        # Assume "time_unit" is in nanoseconds
        cpu_time = benchmark["cpu_time"]

        speed = actual_size * stride / cpu_time
        print(f"{stride} {speed}")

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
