THRESHOLD=$FAILED_TEST_THRESHOLD
if [ -z $THRESHOLD ] || [ ! "$THRESHOLD" =~ ^[0-9]+$ ] ; then
  THRESHOLD=0
fi

TOTAL=0
for log in $TEST_LOGS/*.log; do
  FAILED_TESTS=$(grep ".*failed: \D" $log)
  FAILED_TESTS_COUNT=$(grep ".*failed: \D" $log | wc -l | xargs)
  echo "$(basename $log .filename) failures: $FAILED_TESTS_COUNT\n$FAILED_TESTS\n"
  TOTAL=$((TOTAL+FAILED_TESTS_COUNT))
done

echo "Total test failures: $TOTAL"

if [ $TOTAL -gt $THRESHOLD ]; then
  exit 1
else
  exit 0
fi
