#!/bin/sh

#protoc -I=./ --cpp_out=./ ./addressbook.proto
#protoc -I=./ --cpp_out=./ ./nameinfo.proto

rm *.pb.h *.pb.cc 1>/dev/null 2>&1
for i in *.proto
do
  protoc -I. --cpp_out=. $i
  if [ "$?" -eq 0 ]
  then
    echo "generator $i.pb.h $i.pb.cc"
  else
    exit 1
  fi
done

