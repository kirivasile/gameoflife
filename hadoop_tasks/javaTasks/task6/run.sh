hadoop fs -rm -r output

javac -cp /opt/cloudera/parcels/CDH/lib/hadoop/hadoop-common.jar:/opt/cloudera/parcels/CDH/lib/hadoop-mapreduce/hadoop-mapreduce-client-core.jar -d ./build ./ReplicCount.java

jar cf rc.jar -C ./build ru

hadoop jar rc.jar ru.jiht.ReplicCount input/gore_ot_uma output
