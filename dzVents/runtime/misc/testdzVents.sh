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

NewVersion=$($basedir/domoticz --help | grep Giz | awk '{print $5}') 2>&1 >/dev/null
export NewVersion
if [[ $? -ne 0 ]];then
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

function stopBackgroundProcesses
	{
		pkill node 2>&1 >/dev/null
		pkill domoticz 2>&1 >/dev/null
		if [[ $1 -eq 1 ]] ;then
			exit $1 2>&1 >/dev/null
		fi	
	}

function fillTimes
	{
		Device_ExpectedSeconds=10
		Domoticz_ExpectedSeconds=10
		EventHelpers_ExpectedSeconds=10
		EventHelpersStorage_ExpectedSeconds=20
		HTTPResponse_ExpectedSeconds=20
		ScriptdzVentsDispatching_ExpectedSeconds=10
		TimedCommand_ExpectedSeconds=10
		Time_ExpectedSeconds=10
		Utils_ExpectedSeconds=10
		Variable_ExpectedSeconds=10
		ContactDoorLockInvertedSwitch_ExpectedSeconds=30
		DelayedVariableScene_ExpectedSeconds=80
		EventState_ExpectedSeconds=180
		Integration_ExpectedSeconds=320
		SelectorSwitch_ExpectedSeconds=100
	}

function testDir
	{
		underScore="_ExpectedSeconds"
		testfilelist=(`ls ${prefix}*.lua`)
		outfile=collectOuput
		for i in ${testfilelist[@]}
			do
			testFile=${i%"$suffix"}
			testFile=${testFile#"$prefix"}
			Expected=$(( $(($testFile$underScore)) / factor ))
			echo -n $(showTime) " "
			printf "%-42s %4d  "  $testFile $Expected
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
				printf "%-12s %*s %*s %11.4f" "      OK" 7 $succes 9 $faulty  $timePassed
			fi
			rm $outfile
			echo
		done
	}

stopBackgroundProcesses
pstartSeconds=$SECONDS
prefix="test"
suffix=".lua"
factor=10               # performance factor (about 10 on Raspberry)

cd $basedir/dzVents/runtime/integration-tests
export NODE_NO_WARNINGS=1
npm --silent start 2>&1 >/dev/null &
checkStarted "server" 10

# Just to be sure we do not destroy something important without a backup
cp $basedir/domoticz.db $basedir/domoticz.db_$$

# Make sure we start with a clean sheet
rm -f $basedir/domoticz.db

cd $basedir
./domoticz > domoticz.log$$ &
checkStarted "domoticz" 20

clear
echo "======================== $NewVersion ================+===================== Tests =======================+"
printf "|%6s %11s %80s " "  time |" " test-script"  "| expected | result | successful | failed | seconds |"
echo
echo "==================================================+===================================================+"

fillTimes
totalTest=0
cd $basedir/dzVents/runtime/tests
testDir
cd $basedir/dzVents/runtime/integration-tests
testDir

echo
echo

cd $basedir
expectedErrorCount=5
grep "Results stage 2: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
if [[ $? -eq 0 ]];then
	grep "Results stage 1: SUCCEEDED" domoticz.log$$ 2>&1 >/dev/null
	if [[ $? -eq 0 ]];then
		#echo Stage 1 and stage 2 of integration test Succeeded
		errorCount=$(grep "Error" domoticz.log$$ | grep -v CheckAuthToken | wc -l)
		if [[ $errorCount -eq $expectedErrorCount ]];then
			#echo Errors are to be expected
			echo -n
		else
			grep -i Error  domoticz.log$$ | grep -v CheckAuthToken
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
echo Finished without erors after $(showTime)
stopBackgroundProcesses 0
cleanup
