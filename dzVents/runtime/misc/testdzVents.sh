basedir=${1:-$test}

#
#
# Test script for dzVents under domoticz.
# tested an working on Raspberry Pi with Debian Stretch
# Call with base directory as parm 
#

#
# Root required
#
if [[ $EUID -ne 0 ]]; then
   # This script must be run as root
   echo "This script must be run as root. You are $USER"
   exit 1
fi

busted --version 2>&1 >/dev/null
if [[ $? -ne 0 ]] ;then
   echo "This script requires an installed busted"
   exit 1
fi

which npm 2>&1 >/dev/null
if [[ $? -ne 0 ]] ;then
   echo "This script requires an installed npm (npm install)"
   exit 1
fi

if [ ! -f "$basedir/dzVents/runtime/integration-tests/testIntegration.lua" ]; then
   echo "testIntegration was not found; exiting"
   exit 1
fi


NewVersion=$($basedir/domoticz --help | grep Giz | awk '{print $5}') 2>&1 >/dev/null
export NewVersion
if [[ $? -ne 0 ]] ;then
   echo "This script requires an domoticz"
   exit 1
else
	echo dzVents will be tested against domoticz version $NewVersion.
fi

function leadingZero 
	{
		printf "%02d" $1
	} 

function showTime 
	{
		passedSeconds=$(($SECONDS - $pstartSeconds))
		echo "$(leadingZero  $(($passedSeconds / 3600))):$(leadingZero $(($passedSeconds % 3600 / 60))):$(leadingZero $(($passedSeconds % 60))) "
	}

checkProces()
{
	echo $1
	ps -ef | grep $1 | grep -v 'grep' 2>&1 >/dev/null
	result=$?
}

checkStarted()
{
	process=$1
	maxSeconds=$2
	loopCounter=0
	printf "%s" "Waiting max $maxSeconds seconds for $process to start: " 

	while true; do
		sleep 1
		checkProces $process
		# echo $result $loopCounter $maxSeconds
		if [[ $result -eq 1 && $loopCounter -le $maxSeconds ]];then
			printf "%s" "."
			
			(($loopCounter++))
		else
			if [[ $result -eq 1 ]] ;then
				 echo
				 echo Waited for $loopCounter seconds. I will quit now
				 exit 1
			else
		    	return
			fi
		fi
	done  

} 

function stopBackgroundProcesses 
    {
        pkill node 2>&1 >/dev/null
        pkill domoticz 2>&1 >/dev/null
    }    
    
function testDir()
	{
		testfilelist=(`ls ${prefix}*.lua`)
		for i in ${testfilelist[@]}
		do
			test=${i%"$suffix"}
			test=${test#"$prefix"}
			echo -n $(showTime) " "
			printf "%-44s" " $test" ; busted $i  2>&1 >/dev/null
			if [[ $? -ne 0 ]] ;then
				printf "%-10s" "===>> NOT OK" 
				echo 
				echo
				busted -o TAP $i.lua | grep "not ok"
				echo
			else
				printf "%-10s"  "===>> OK" 
			fi     
			echo
		done 
	}

stopBackgroundProcesses 

pstartSeconds=$SECONDS
prefix="test"
suffix=".lua"


cd $basedir/dzVents/runtime/integration-tests
npm --silent start 2>&1 >/dev/null & 
checkStarted "server" 10

# Just to be sure we do not destroy something important without a backup
cp $basedir/domoticz.db $basedir/domoticz.db_$$

cd $basedir
./domoticz > domoticz.log$$ & 
checkStarted "domoticz" 20

clear
echo "======================== $NewVersion ============================="
printf "|%6s %11s %40s" "  time |" " test-script"   "  | result |"; 
echo
echo "==============================================================="

cd $basedir/dzVents/runtime/tests
testDir
cd $basedir/dzVents/runtime/integration-tests
testDir


cd $basedir
grep "Results stage 2: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
if [[ $? -eq 0 ]] ;then
    grep "Results stage 1: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
    if [[ $? -eq 0 ]] ;then
        #echo Stage 1 and stage 2 of integration test Succeeded
        echo -n
    else
        echo Stage 1 of integration test was not completely succesfull
        grep -i fail domoticz.log$$ 
        grep -i 'not ok' domoticz.log$$ 
        stopBackgroundProcesses 1
    fi
else
   echo Stage 2 of integration test was not completely succesfull
   grep -i fail domoticz.log$$ 
   grep -i 'not ok' domoticz.log$$ 
   stopBackgroundProcesses 1
fi

echo 
echo Finished after $(showTime)
stopBackgroundProcesses 
        
