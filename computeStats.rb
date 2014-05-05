#!/usr/bin/ruby
require 'optparse'

$options = {
    :nameProg => "./morpionSol",
    :nbrun => 100,
    :keyWord => ["Score","Playout"],
    :nbSignificantNumbers => 1
}
$options[:format] = "%." + $options[:nbSignificantNumbers].to_s + "f"

optparse = OptionParser.new do |opt|
    opt.banner = "usage: ./computeStats.rb [options]"

    opt.on( '-p', '--prog NP', 'executable to run') do |name|
        $options[:nameProg] = name
    end

    opt.on('-n', '--nbrun NB',Integer, 'nbrun') do |nb|
        $options[:nbrun] = nb
    end

    opt.on('-k', '--keyword key1,key2,key3', Array, 'list of keywords to find in the output') do |l|
        $options[:keyWord] = l
    end

    opt.on('-sn', '--signum SN',Integer, 'number of significant numbers in the result') do |sn|
        $options[:nbSignificantNumbers] = sn 
        $options[:format] = "%." + $options[:nbSignificantNumbers].to_s + "f"
    end

    opt.on( '-h', '--help', 'Display this screen' ) do
        puts opt
        exit
    end
end

optparse.parse!

if (!ARGV.empty?)
    STDERR.puts "argument " + ARGV.inspect + " is not recognized"
    exit
end

def launchProg()
    cmd = $options[:nameProg]
    value = `#{cmd}`
    if (!$?.success?)
        STDERR.puts ""
        STDERR.puts "program \"" + cmd + "\" not found"
        exit
    end
    res = {}
    $options[:keyWord].each do |keyword|
        tempRes = value.scan(/#{keyword}.* (\d*)/)   # res is a list of all the occurence of keyword
                                                     # the value is the last number on a line where keyword is found
        if (tempRes.length == 0)
            STDERR.puts ""
            STDERR.puts "keyword \"#{keyword}\" not found during the execution of the program"
            exit
        end
        res.store(keyword,tempRes)   
    end
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


def printRes(listRes,listTime)
    l1 = "#"
    l2 = "#"
    $options[:keyWord].each do |key|
        l1 += key.to_s + "\t\t"
        l2 += "mean\t" + "ci\t"
    end
    l1 += "Time"
    puts l1
    puts l2
    nbLines = listRes.values.map{|x| x.length}.max
    for i in 0...nbLines do
        s = ""
        $options[:keyWord].each do |key|
            if (i < listRes[key].length)
                s += $options[:format] % meanArray(listRes[key][i]).to_s + "\t" + $options[:format] % ciArray(listRes[key][i]).to_s + "\t" #+ sdArray(listRes[key][i]).to_s + "\t" + listRes[key][i].max.to_s + "\t"
            else
                s += $options[:format] % meanArray(listRes[key][-1]).to_s + "\t" + $options[:format] % ciArray(listRes[key][-1]).to_s + "\t" # + sdArray(listRes[key][-1]).to_s + "\t" + listRes[key][-1].max.to_s + "\t"
            end
        end
        s += $options[:format] % meanArray(listTime).to_s 
        puts s
    end
    #puts meanArray(listTime).to_s + "s"
end

begin

    #init
    listRes = {}
    $options[:keyWord].each do |key|
        listRes.store(key,[])
    end
    listTime = []

    for i in 1..$options[:nbrun]
        STDERR.printf("\r%i / %i",i,$options[:nbrun] )
        STDERR.flush
        startt = Time.now
        res = launchProg()

        # if a new results has more values for a certain key, add new rows in listRes
        res.keys.each do |resKey|
            if (res[resKey].length>listRes[resKey].length)
                for j in listRes[resKey].length...res[resKey].length
                    listRes[resKey].push([])
                end
            end
        end

        # add the results to listRes
        listRes.keys.each do |resKey|
            for j in 0...res[resKey].length
                listRes[resKey][j].push(res[resKey][j][0].to_i)
            end
        end

        endt = Time.now
        listTime.push(endt-startt)
    end
    STDERR.puts ""
    printRes(listRes,listTime)

rescue Exception => e
    STDERR.puts ""
    if (i>2)
        printRes(listRes,listTime)
    else
        puts "not enough stats"
    end
end

