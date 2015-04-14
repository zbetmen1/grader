<?php
  include("grader_php.php");
  
  // Read source file
  $srcPath = "/home/zbetmen/Documents/programming/grader/mod_grader/src/examples/c/test_timeout.c";
  $srcHandle = fopen($srcPath, "r");
  $srcName = "test_timeout.c";
  $srcCont = fread($srcHandle, filesize($srcPath));
  fclose($srcHandle);
  
  // Read test file
  $testPath = "/home/zbetmen/Documents/programming/grader/mod_grader/src/examples/c/test_timeout.xml";
  $testHandle = fopen($testPath, "r");
  $testCont = fread($testHandle, filesize($testPath));
  
  // Create new task
  $taskId = grader_php::submit_new_task($srcName, $srcCont, $testCont);
  if ($taskId != null)
  {
    while (true)
    {
      sleep(1);
      $status = grader_php::get_task_status($taskId);
      
      echo "Status is: ". $status . "\n";
      $statusObj = json_decode($status);
      if ($statusObj->STATE == "TIME_LIMIT" || 
          $statusObj->STATE == "FINISHED" || 
          $statusObj->STATE == "INVALID" ||
          $statusObj->STATE == "COMPILE_ERROR")
          break;
    }
    grader_php::destroy_task($taskId);
  }
  else
  {
    echo "Ooops taskId is null!\n";
  }
?>