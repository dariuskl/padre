#!/usr/bin/env sh

begin_test_case () {
  printf "\t%s" "$1"
}

end_test_case () {
  if test $? -eq 0; then
    printf "\r OK \n"
  else
    printf "\r FAIL \n"
  fi
}

begin_test_case "sha256 implementation is sane"
test "$(echo abc | sha256sum)" = "$(echo abc | ./build/test_sha256)"
end_test_case

begin_test_case "internal padre unit tests pass"
./build/test_padre > /dev/null
end_test_case

begin_test_case "WHEN calling without arguments THEN command fails"
test ! "$(./build/padre > /dev/null)"
end_test_case

begin_test_case "WHEN calling without arguments THEN prints error & usage"
expect -c '
  spawn ./build/padre
  expect {
    default                 { exit 1 }
    "error: *\nusage: *"    { exit 0 }
  }
' > /dev/null
end_test_case

begin_test_case "WHEN calling with too many arguments THEN command fails"
test ! "$(./build/padre 1 2 3 > /dev/null)"
end_test_case

begin_test_case "WHEN calling with too many arguments THEN prints error & usage"
expect -c '
  spawn ./build/padre 1 2 3
  expect {
    default                 { exit 1 }
    "error: *\nusage: *"    { exit 0 }
  }
' > /dev/null
end_test_case

begin_test_case "WHEN calling with domain and username THEN yields password"
expect -c '
  spawn ./build/padre example.com user -l 8 -c a-z
  expect "Enter the master password:" { send -- "123\n" }
  expect "wmewulab" { exit 0 }
  exit 1
' > /dev/null
end_test_case

begin_test_case "WHEN given a correct account entry THEN is parsed correctly"
expect -c '
  spawn $env(SHELL) -c "echo example.com,user,0,8,a-z | ./build/padre -"
  expect "Enter the master password:" { send -- "123\n" }
  expect "wmewulab" { exit 0 }
  exit 1
' > /dev/null
end_test_case
