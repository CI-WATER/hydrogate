#!/bin/bash
#Test script for return_hpc_program_names method
echo "Enter token: "
read mytoken
echo "Enter HPC name: "
read hpcname
curl -k -X POST --data "token=$mytoken&hpc=$hpcname" https://127.0.0.1/hydrogate/return_hpc_program_names/
echo ""
