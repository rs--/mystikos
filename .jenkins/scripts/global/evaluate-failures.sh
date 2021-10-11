THRESHOLD=$FAILED_TEST_THRESHOLD
if [ -z $THRESHOLD ] || [ ! "$THRESHOLD" =~ ^[0-9]+$ ] ; then
  THRESHOLD=0
fi

FAILED_TESTS=$(grep "failed: tests" $TEST_LOG | wc -l | xargs)

if [ $FAILED_TESTS -gt $THRESHOLD ] ; then
  echo "Failed tests ($FAILED_TESTS) exceeds threshold ($THRESHOLD), exiting"
  exit 1
else
  echo "Failed tests ($FAILED_TESTS) within threshold ($THRESHOLD), continuing"
fi
