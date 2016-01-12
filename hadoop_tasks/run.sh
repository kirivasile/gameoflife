hadoop fs -rm -r output
hadoop jar /opt/cloudera/parcels/CDH-5.5.1-1.cdh5.5.1.p0.11/lib/hadoop-mapreduce/hadoop-streaming.jar \
-input input/* \
-output output \
-mapper ./mapper.py \
-reducer ./reducer.py \
-file ./mapper.py \
-file ./reducer.py \
-file /opt/cloudera/parcels/CDH-5.5.1-1.cdh5.5.1.p0.11/lib/hadoop-mapreduce/hadoop-streaming.jar \
-numReduceTasks 1
