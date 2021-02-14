basedir=${1:-$test}
#
#
# Test script for dzVents under domoticz.
# tested and working on Raspberry Pi with Debian Buster
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
if [[ $? -ne 0 ]];then
	echo "This script requires an installed busted"
	exit 1
fi

which npm 2>&1 >/dev/null
if [[ $? -ne 0 ]];then
	echo "This script requires an installed npm (npm install)"
	exit 1
fi

if [ ! -f "$basedir/dzVents/runtime/integration-tests/testIntegration.lua" ]; then
	echo "testIntegration was not found; exiting"
	exit 1
fi

NewVersion=$($basedir/domoticz --help | grep Giz | awk '{print $5 " (" $7}') 2>&1 >/dev/null
export NewVersion
if [[ $? -ne 0 ]];then
	echo "This script requires an domoticz"
	exit 1
else
	echo dzVents will be tested against domoticz version $NewVersion.
fi

currenttime=$(date +%H:%M)
if [[ "$currenttime" > "00:00" ]] && [[ "$currenttime" < "00:31" ]]; then
	echo Script cannot be execute dbetween 00:00 and 00:30. Time checks will fail
	exit 1
fi

dzVersion=$(grep DZVERSION dzVents/runtime/Utils.lua | head -1 | grep -oP "'.*'"  | sed s/\'//g)

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
			loopCounter=$((loopCounter+1))
		else
			if [[ $result -eq 1 ]];then
				 echo
				 echo Waited for $loopCounter seconds. I will quit now
				 exit 1
			else
				return
			fi
		fi
	done
}

function cleanup
	{
		ps -aux | grep [f]domoticz
		if [[ $? -eq 1 ]]; then
			rm domoticz.log[0-9][0-9]*
			find . -type f -name 'domoticz.db_*' -mmin +30 -exec rm {} \;
		fi
	}

function killDomoticz
	{
		sleep 3
		kill -9 $(cat domoticz.pid) 2>&1 >/dev/null
	}

function stopBackgroundProcesses
	{
		sleep 3
		pkill node  2>&1 >/dev/null
		pkill domoticz  2>&1 >/dev/null
		if [[ $1 -eq 1 ]] ;then
			exit $1 2>&1 >/dev/null
		fi
	}

function fillTimes
	{
		Device_ExpectedSeconds=10
		Domoticz_ExpectedSeconds=10
		EventHelpers_ExpectedSeconds=10
		EventHelpersStorage_ExpectedSeconds=10
		HTTPResponse_ExpectedSeconds=10
		Lodash_ExpectedSeconds=10
		ScriptdzVentsDispatching_ExpectedSeconds=10
		TimedCommand_ExpectedSeconds=10
		Time_ExpectedSeconds=30
		Utils_ExpectedSeconds=10
		Variable_ExpectedSeconds=10
		ContactDoorLockInvertedSwitch_ExpectedSeconds=30
		DelayedVariableScene_ExpectedSeconds=80
		EventState_ExpectedSeconds=190
		Integration_ExpectedSeconds=340
		SelectorSwitch_ExpectedSeconds=100
		SystemAndCustomEvents_ExpectedSeconds=10
	}

function fillNumberOfTests
	{
		Device_ExpectedTests=119
		Domoticz_ExpectedTests=85
		EventHelpers_ExpectedTests=34
		EventHelpersStorage_ExpectedTests=50
		HTTPResponse_ExpectedTests=6
		Lodash_ExpectedTests=100
		ScriptdzVentsDispatching_ExpectedTests=2
		TimedCommand_ExpectedTests=46
		Time_ExpectedTests=369
		Utils_ExpectedTests=36
		Variable_ExpectedTests=15
		ContactDoorLockInvertedSwitch_ExpectedTests=2
		DelayedVariableScene_ExpectedTests=2
		EventState_ExpectedTests=2
		Integration_ExpectedTests=222
		SelectorSwitch_ExpectedTests=2
		SystemAndCustomEvents_ExpectedTests=9
	}

function testDir
	{
		underScoreSeconds="_ExpectedSeconds"
		underScoreTests="_ExpectedTests"
		testfilelist=(`ls ${prefix}*.lua`)
		outfile=collectOuput
		for i in ${testfilelist[@]}
			do
			testFile=${i%"$suffix"}
			testFile=${testFile#"$prefix"}
			Expected=$(( $(($testFile$underScoreSeconds)) / factor ))
			Tests=$(( $(($testFile$underScoreTests)) / 1 ))
			if test $i == 'testSystemAndCustomEvents.lua' ; then
				tput sc # save cursor
				stopBackgroundProcesses
				sleep 6
				tput rc;tput el # rc
			fi
			echo -n $(showTime) " "
			printf "%-42s %4d "  $testFile $Expected
			printf "%10d" $Tests
			# busted $i  2>&1 >/dev/null
			busted $i  > $outfile

			if [[ $? -ne 0 ]];then
				printf "%-10s" "===>> NOT OK"
				echo
				echo
				busted -o TAP $i.lua | grep "not ok"
				echo
			else
				bottomLine=$(tail -1 $outfile)
				succes=$(echo $bottomLine |awk '{print $1}')
				failed=$(echo $bottomLine |awk '{print $4}')
				error=$(echo $bottomLine |awk '{print $7}')
				timePassed=$(echo $bottomLine |awk '{print $13}')
				totalTest=$((totalTest + succes))
				faulty=$((failed + error))
				printf "%-12s %*s %*s %11.4f" "	  OK" 7 $succes 9 $faulty  $timePassed
			fi
			rm $outfile
			echo
		done
	}

stopBackgroundProcesses
pstartSeconds=$SECONDS
prefix="test"
suffix=".lua"
factor=10			   # performance factor (about 10 on Raspberry)

cd $basedir/dzVents/runtime/integration-tests
export NODE_NO_WARNINGS=1
npm --silent start --scripts-prepend-node-path 2>&1 >/dev/null &
checkStarted "server" 10

# Just to be sure we do not destroy something important without a backup
cp $basedir/domoticz.db $basedir/domoticz.db_$$

# Make sure we start with a clean sheet
rm -f $basedir/domoticz.db

cd $basedir
cp dzVents/runtime/integration-tests/scriptTestCustomAndSystemEventsScript.lua scripts/dzVents/scripts/scriptTestCustomAndSystemEventsScript.lua

cd $basedir
./domoticz  -www 8080 -sslwww 444 -pidfile domoticz.pid --> domoticz.log$$ &
checkStarted "domoticz" 20

clear
echo "======== domoticz $NewVersion, dzVents V$dzVersion +========================== Tests ==============================+"
echo "  time  | test-script	                           | expected | tests | result  | successful |  failed  | seconds  |"
echo "===================================================+===============================================================+"

fillTimes
fillNumberOfTests
totalTest=0
cd $basedir/dzVents/runtime/tests
testDir
cd $basedir/dzVents/runtime/integration-tests
testDir

echo
echo

cd $basedir
expectedErrorCount=6
grep "Results stage 2: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
if [[ $? -eq 0 ]];then
	grep "Results stage 1: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
	if [[ $? -eq 0 ]];then
		#echo Stage 1 and stage 2 of integration test Succeeded
		errorCount=$(grep "Error" domoticz.log$$ | grep -v CheckAuthToken | grep -v errorText | wc -l)
		errorLine=$(grep "Error" domoticz.log$$ | grep -v CheckAuthToken | grep -v errorText | head -7 | tail -1 | grep -v Segmentation | wc -l)
		if [ $errorCount -le $expectedErrorCount ] || [ $errorLine -eq 0 ] ;then
			#echo Errors are to be expected
			echo -n
		else
			grep -i Error  domoticz.log$$ | grep -v CheckAuthToken | grep -v LOG_ERROR | grep -v errorText
			stopBackgroundProcesses 1
		fi
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

echo Total tests: $totalTest
echo $(date)';' dzVents version $dzVersion tested without errors after $(showTime)
stopBackgroundProcesses 0
cleanup
