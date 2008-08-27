#!/usr/bin/ruby

require 'syslog'

@config_file="xerxes.conf"
@bin_name="./xerxes"

@log = Syslog.open('xerxes')

@threads = Array.new ;

def run_process(cfg, num) 
   m = Thread.current
   t = Thread.start{

    pid = Process.fork{
      exec(@bin_name, "--src", cfg[:src], "--dst", cfg[:dst], (cfg[:debug] ? "--debug" : ""), (cfg[:quiet] ? "--quiet" : ""))
    }
    @threads[num][:pid] = pid
    @log.notice("Xerxes for #{cfg[:src]}<->#{cfg[:dst]} started")
    Process.wait(pid)
    @log.warning("Xerxes for #{cfg[:src]}<->#{cfg[:dst]} died")
    sleep 0.2
    m.wakeup 
  }
  t
end

def check_threads
  @threads.each_index{|idx|
    if (!@threads[idx][:thread].alive?)
      @log.notice("try to restart Xerxes for #{@threads[idx][:config][:src]}<->#{@threads[idx][:config][:dst]}")
      @threads[idx][:thread] = run_process(@threads[idx][:config], idx)
    end
  }
end

i = 0;
File.foreach(@config_file){|line|
  if (line =~ /^((tcp|unix):([\d\w_.\/-]+)(:(\d+))?) ((tcp|unix):([\d\w_.\/-]+)(:(\d+))?)( quiet)?( debug)?$/)
    
    match = Regexp.last_match[1..12]

    @threads[i] = Hash.new
    @threads[i][:config] = Hash.new

    @threads[i][:config][:src] = match[0]
    @threads[i][:config][:dst] = match[5]
    @threads[i][:config][:quiet] = ! match[10].nil?
    @threads[i][:config][:debug] = ! match[11].nil?

    i = i +1
  end
}

@threads.each_index{|idx|
  @log.notice("try to start Xerxes for #{@threads[idx][:config][:src]}<->#{@threads[idx][:config][:dst]}")
  @threads[idx][:thread] = run_process(@threads[idx][:config], idx)
}

m = Thread.current
Thread.start{
  Signal.trap("TERM"){
    @log.warning("SIGTERM recived, exiting")
    @threads.each{|thread|
      @log.info("quit Xerxes for #{thread[:config][:src]}<->#{thread[:config][:dst]}")
      thread[:thread].kill
      Process.kill("TERM", thread[:pid])
    }
    Process.exit!
  }
}

while(true) do
  sleep
  sleep 0.5
  check_threads
end
