#!/bin/bash

complete_backup_count=2000
incremental_backup_count=1000

# Function to create a complete backup
create_complete_backup() {
    local backup_file=cb${complete_backup_count}.tar

    # Find all .txt files and create a tar archive
    find ~/ -type f -name "*.txt" -exec tar -rvf ~/home/backup/cb/${backup_file} {} + >/dev/null 2>&1

    # Update backup.log with the timestamp and the name of the tar file
    echo "$(date +'%a %d %b %Y %I:%M:%S %p %Z') ${backup_file} was created" >> ~/home/backup/backup.log
    complete_backup_count=$((complete_backup_count+1))
}

# Function to create an incremental backup
create_incremental_backup() {
    local backup_file=ib${incremental_backup_count}.tar

    # Find all .txt files modified since the last complete or incremental backup
    local files=$(find ~/ -name "*.txt" -newermt "$(stat -c %y $(ls -t ~/home/backup/cb/* ~/home/backup/ib/* | head -1))" 2>/dev/null)

    if [ -n "$files" ]; then
        # Create a tar archive of the modified files
        echo "$files" | xargs tar -cvf ~/home/backup/ib/${incremental_backup_count} >/dev/null 2>&1

        # Update backup.log with the timestamp and the name of the tar file
        echo "$(date +'%a %d %b %Y %I:%M:%S %p %Z') ${backup_file} was created" >> ~/home/backup/backup.log
        incremental_backup_count=$((incremental_backup_count+1))
    else
        # Update backup.log with the timestamp and a message
        echo "$(date +'%a %d %b %Y %I:%M:%S %p %Z') No changes-Incremental backup was not created" >> ~/home/backup/backup.log
    fi
}

run_backup() {
    mkdir -p ~/home/backup/cb ~/home/backup/ib

    # Main loops
    while true; do
        create_complete_backup
        sleep 120

        create_incremental_backup
        sleep 120

        create_incremental_backup
        sleep 120

        create_incremental_backup
        sleep 120
    done
}

run_backup > /dev/null 2>&1 &