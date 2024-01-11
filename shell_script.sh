
#!/bin/bash

cd ~/Desktop
gcc main.c report_email_sender.c cJSON.c -o test -lcurl
if [ $? -eq 0 ]; then
  echo "Compilation successful. Running the program..."
  
  
  ./test
else
  echo "Compilation failed. Please check the errors."
fi
