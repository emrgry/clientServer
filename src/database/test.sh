for file in "."/*.pck; do
        if [ -f "$file" ]; then
            # Get the creation date of the file
            # For mac:
            #creation_date=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$file")
            # For linux: -c %Y
            creation_date=$(stat -f %m "$file")  
            file_size=$(ls -lT "$file" | awk '{print $5}')          # Print the filename with its creation date
            echo "$file ==> $creation_date $file_size" >> ./package_list.txt
            fi
        done