// Project headers
#include "daemon.hpp"
#include "network/tcp_server.hpp"
#include "logger.hpp"
#include "configuration.hpp"

// BOOST headers
#include <boost/filesystem.hpp>

// Unix headers
#include <pwd.h>

using namespace std;
namespace fs = boost::filesystem;

//////////////////////////////////////////////////////////////////////////////
// Daemon implementation
//////////////////////////////////////////////////////////////////////////////
namespace grader 
{
  daemon::daemon(unsigned n, const string& workingDir, function<void(int)> f)
  : m_mainLoop(f)
  {
    // Set working directory
    if (!workingDir.empty())
    {
      if (::chdir(workingDir.c_str()) == -1)
      {
        string msg = "Changing work directory in daemon construction failed! Message: '";
        msg += ::strerror(errno);
        msg += "'.";
        THROW_SMART(daemon_exception, msg);
      }
    }
    
    // Daemonize current process
    if (::daemon(no_dir_change, std_fds_to_dev_null) == -1)
    {
      string msg = "Failed to daemonize process! Message: '";
      msg += ::strerror(errno);
      msg += "'.";
      THROW_SMART(daemon_exception, msg);
    }
    
    // Start workers
    m_workers.reserve(n);
    for (unsigned i = 0; i < n; ++i)
      m_workers.emplace_back(bind(m_mainLoop, i));
    
    // Set handlers
    set_handlers(&handle_daemon_termination, &handle_child_failure, &handle_reinitialization);
    
    glog_st.log(severity::info, "Daemon started pid=", ::getpid(), ".");
  }
  
  daemon& daemon::instance(unsigned n, const string& workingDir, function<void(int)> f)
  {
    static daemon d(n, workingDir, f);
    return d;
  }
  
  void daemon::set_handlers(::sighandler_t termination, 
                            ::sighandler_t failure, 
                            ::sighandler_t reinitialization) const
  {
    if (::signal(SIGTERM, termination) == SIG_ERR)
    {
      string msg = "Failed to initialize termination handler! Message: '";
      msg += ::strerror(errno);
      msg += "'.";
      THROW_SMART(daemon_exception, msg);
    }
    
    if (::signal(SIGCHLD, failure) == SIG_ERR)
    {
      string msg = "Failed to initialize child failure handler! Message: '";
      msg += ::strerror(errno);
      msg += "'.";
      THROW_SMART(daemon_exception, msg);
    }
    
    if (::signal(SIGHUP, reinitialization) == SIG_ERR)
    {
      string msg = "Failed to initialize configuration reload handler! Message: '";
      msg += ::strerror(errno);
      msg += "'.";
      THROW_SMART(daemon_exception, msg);
    }
  }
  
  void daemon::handle_daemon_termination(int)
  {
    if (::signal(SIGCHLD, SIG_IGN) == SIG_ERR)
      glog_st.log(severity::error, "Couldn't set signal handler for child exit to be SIG_IGN. It is possible that children will continue to run.");
    exit(EXIT_SUCCESS);
  }
  
  void daemon::handle_child_failure(int)
  {
    daemon& d = daemon::instance();
    boost::optional<int> exitCode;
    for (size_t i = 0; i < d.m_workers.size(); ++i)
    {
      if ((exitCode = d.m_workers[i].finished()))
      {
        glog_st.log(severity::error, "Child failed with exit code '", exitCode.get() ,"' Worker id=", i);
        d.m_workers[i] = move(process(bind(d.m_mainLoop, i)));
        glog_st.log(severity::info, "Worker ", i, " re-spawned!");
      }
    }
  }
  
  void daemon::handle_reinitialization(int)
  {
    daemon& d = daemon::instance();
    for (process& worker : d.m_workers)
      worker.send_signal(SIGHUP);
  }
  
  void main_loop(int workerIdx)
  {
    // Get worker user
    configuration& conf = configuration::instance();
    string workerUser = conf.get($(worker_user_base_name)) + to_string(workerIdx);
    struct passwd* pwWorkerUser = ::getpwnam(workerUser.c_str());
    
    // Create worker user if not exists
    if (!pwWorkerUser && errno != 0)
    {
      glog_st.log(severity::fatal, "Getting worker user '", workerUser, "' failed. Message: '", ::strerror(errno), "'.");
      exit(EXIT_FAILURE);
    }
    else if (!pwWorkerUser && errno == 0)
    {
      // Create worker user
      string homeDir = conf.get($(work_dir)) + '/' + workerUser;
      string cmd = "useradd -g " + conf.get($(worker_user_group)) +        // Create user with group
                   " -m -d " + homeDir + // Create home directory
                   " -r -s /bin/false " + workerUser;                      // Create system user with no terminal
      int exitVal = system(cmd.c_str());
      if (exitVal != 0)
      {
        glog_st.log(severity::fatal, "Couldn't create user: ", workerUser, 
                    ". Exit code for system command: ", exitVal, ".");
        exit(EXIT_FAILURE);
      }
      
      // Get worker used passwd
      if (!(pwWorkerUser = ::getpwnam(workerUser.c_str())))
      {
        glog_st.log(severity::fatal, "Couldn't get pointer to struct passwd for successfully created user. User: ",
                    workerUser, ". Message: '", ::strerror(errno), "'.");
        exit(EXIT_FAILURE);
      }
    }
    uid_t workerUserId = pwWorkerUser->pw_uid;
    glog_st.log(severity::info, "Starting worker idx=", workerIdx, " pid=", ::getpid(), " under user id=", workerUserId, "'.");
    
    // Get working directory
    fs::path workDir = conf.get($(work_dir));
    workDir /= workerUser;
    
    // Change working directory
    if (::chdir(workDir.c_str()) == -1)
    {
      glog_st.log(severity::fatal, "Failed to change directory in worker. Message: '", workDir.string(), "'.");
      exit(EXIT_FAILURE);
    }
    
    // TODO: Process tasks
    ::pause();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Main function
//////////////////////////////////////////////////////////////////////////////
int main(int, char**) 
{
  try
  {
    // Create daemon workers
    grader::configuration& conf = grader::configuration::instance();
    grader::daemon::instance(stoi(conf.get($(worker_num))), conf.get($(work_dir)), &grader::main_loop);
    
    // Start tcp server
    aio::io_service service;
    grader::tcp_server server(service);
    service.run();
  } catch (exception& e)
  {
    grader::glog_st.log(grader::severity::fatal, "Daemon failed to start! Message: '", e.what(), "'.");
  }
}