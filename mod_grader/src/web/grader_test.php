<?php
  include("grader_php.php");
  
  // Read source file
  $srcPath = "/home/zbetmen/Documents/programming/grader/mod_grader/src/examples/java/StdStd.java";
  $srcHandle = fopen($srcPath, "r");
  $srcName = "StdStd.java";
  $srcCont = fread($srcHandle, filesize($srcPath));
  fclose($srcHandle);
  
  // Read test file
  $testPath = "/home/zbetmen/Documents/programming/grader/mod_grader/src/examples/java/std_std.xml";
  $testHandle = fopen($testPath, "r");
  $testCont = fread($testHandle, filesize($testPath));
  
  // Create new task
  $taskId = grader_php::submit_new_task($srcName, $srcCont, $testCont);
  if ($taskId != null)
  {
    sleep(2);
    echo "Status is: ". grader_php::get_task_status($taskId). "\n";
    grader_php::destroy_task($taskId);
  }
  else
  {
    echo "Ooops taskId is null!\n";
  }
?>