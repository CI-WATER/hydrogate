#!/bin/bash
#Test script for retrieve_job_status method
echo "Enter token: "
read mytoken
echo "Enter job id: "
read myjobid
curl -k -X GET "https://127.0.0.1/hydrogate/retrieve_job_status?token=$mytoken&jobid=$myjobid"
echo ""
