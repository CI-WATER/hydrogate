#!/bin/bash
#Test script for submit job method
echo "Enter token: "
read mytoken
echo "Enter package id: "
read packageid
# find sample job description texts on the sample_job_description_files folder
echo "Enter job definition: " 
read jobdef
curl -k -X POST --data "token=$mytoken&packageid=$packageid&jobdefinition=$jobdef" https://127.0.0.1/hydrogate/submit_job/
echo ""
