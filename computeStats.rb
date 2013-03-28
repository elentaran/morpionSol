#!/usr/bin/ruby

$nameProg = "./morpionSol"

usage = "usage: computeStats.rb nbrun[=100]"

if (ARGV.length > 0)
    nbrun = ARGV[0].to_i
    if (nbrun == 0)
        puts usage
    end
else
    nbrun = 100
end

if (ARGV.length > 1)
    puts usage
end

def launchProg()
    cmd = $nameProg
    value = `#{cmd}`
    res = value.scan(/Score: (\d*)/)[0][0].to_i   # res is the first (and only) match
    return res
end

def meanArray(list)
        return (list.reduce(:+)).to_f / list.size
end

def sdArray(list)
    m = meanArray(list)
    variance = list.inject(0) { |variance, x| variance += (x - m) ** 2 }
    return Math.sqrt(variance/(list.size-1))
end

def ciArray(list)
    s = sdArray(list)
    return 1.96*s/Math.sqrt(list.size)
end

begin

listRes = []
listTime = []
puts "coucou"
for i in 1..nbrun
    printf("\r%i / %i",i,nbrun )
    STDOUT.flush
    startt = Time.now
    res = launchProg()
    listRes.push(res)
    endt = Time.now
    listTime.push(endt-startt)
end
puts ""
puts meanArray(listRes).to_s + " +- " + ciArray(listRes).to_s + " (" + meanArray(listTime).to_s + "s)"
rescue Exception => e
    puts ""
    if (i>2)
        puts meanArray(listRes).to_s + " +- " + ciArray(listRes).to_s + " (" + meanArray(listTime).to_s + "s)"
    else
        puts "not enough stats"
    end
end

