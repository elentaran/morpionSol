#!/usr/bin/ruby

##### those variable need to be configured for the script to work
#####
# command to execute. put "PARAM" where the parameter is used.
#$execProg = 'make opt CPPFLAG="PARAM"; ./latinSquare'     
#$execProg = 'make opt CPPFLAG="PARAM" -s; ./computeStats.rb 10'     
$execProg = './computeStats.rb -n 1 -p "./morpionSol PARAM"'     
#$execProg = 'mkdir "tempPARAM"; cp *.cpp *.h Makefile computeStats.rb "tempPARAM"; cd "tempPARAM"; make opt CPPFLAG="PARAM"; ./computeStats.rb 1000; cd ..; rm -r "tempPARAM"'     
#$execProg = './computeStats.rb 100 "./latinSquare -probaKeep PARAM"'
#$execProg = './computeStats.rb 10'     

# list of parameter values. a..b for all the values between a and b or [a,b,c] for the values a, b and c.
#$paramValues = ["-DDIM=4 -DLARGEUR=25","-DDIM=9 -DLARGEUR=10","-DDIM=8 -DLARGEUR=20" ,"-DDIM=3 -DLARGEUR=20","-DDIM=2 -DLARGEUR=20"]       
#$paramValues = ["-DDIM=4 -DLARGEUR=25","-DDIM=9 -DLARGEUR=10","-DDIM=8 -DLARGEUR=20"]
#param1 = ["-DDIM="].product((3..10).to_a).map(&:join)
#param2 = [" -DLARGEUR="].product((2..25).to_a).map(&:join)
#$paramValues = param1.product(param2).map(&:join)
#$paramValues = (0..10).to_a
#$paramValues = [2,3]
$paramValues = ["-level 1 -nbSearches 1000000", "-level 2 -nbSearches 1000", "-level 3 -nbSearches 100", "-level 4 -nbSearches 32"]
#$paramValues = ["-DDIM=4 -DLARGEUR=25","-DDIM=9 -DLARGEUR=10","-DDIM=8 -DLARGEUR=20"]
#param1 = ["-DLAMBDA=63 -DGEN=158", "-DLAMBDA=200 -DGEN=500", "-DLAMBDA=630 -DGEN=1580"]
#param2 = [" -DDIM=4 -DLARGEUR=25"," -DDIM=9 -DLARGEUR=10"," -DDIM=8 -DLARGEUR=20"]
#$paramValues = param1.product(param2).map(&:join)

# name of the parameter
$paramName = "param"

# name of the file where the results will be stored (if empty, results will be printed on screen).
$fileRes = "nrpa.data"             

# number of cores that should be used
$nbcores = 1
#####



def execCmd(param)
    execCmd = $execProg
    if (execCmd.include?("PARAM"))
        execCmd = execCmd.gsub(/PARAM/, "#{param}")
    end
    puts execCmd
    value = `#{execCmd}`
    if (!$?.success?)
        puts "error during the execution"
        exit
    else
        STDERR.puts $paramName + " = " + param.to_s + "done (" + $paramValues.length.to_s + " left in queue)"
        if ($fileRes.empty?)
            puts "#" + $paramName + " = " + param.to_s
            puts value
            puts "\n\n"
        else
            f = File.open($fileRes,'a')
            f.puts "#" + $paramName + " = " + param.to_s
            f.puts value
            f.puts "\n\n"
            f.close
        end
    end
end

def func()
    while (!$paramValues.empty?) do
        param = $paramValues.first
        $paramValues = $paramValues.drop(1)
        execCmd(param)
    end
end

listThread = []
for i in 1..$nbcores do
    listThread[i] = Thread.new{func()}
end

for i in 1..$nbcores do
    listThread[i].join
end


