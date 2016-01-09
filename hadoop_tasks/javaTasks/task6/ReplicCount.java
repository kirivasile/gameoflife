package ru.jiht;

import java.io.IOException;
import java.util.*;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;

public class ReplicCount  {
	public static class Map extends Mapper<LongWritable, Text, Text, IntWritable> {
		private final static IntWritable one = new IntWritable(1);
		private Text word = new Text();
	
		public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
			String line = value.toString().trim();
			/*StringTokenizer tokenizer = new StringTokenizer(line);
			while (tokenizer.hasMoreTokens()) {
		 		word.set(tokenizer.nextToken());
				context.write(word, one);
 			}*/
			if (line.startsWith("<td id=\"LC")) {
				String reply = line.substring(line.indexOf(">") + 1, line.lastIndexOf("<"));
				if (!reply.startsWith("==")) {
					String author = "";
					if (reply.substring(0, 1).matches("\\d")) {
						author = reply.split("\\s+")[0] + " " + reply.split("\\s+")[1];
					} else {
						author = reply.split("\\s+")[0];
					}
					word.set(author);
					context.write(word, one);
				}
			}	
 		}
	}

	public static class Reduce extends Reducer<Text, IntWritable, Text, IntWritable> {
		public void reduce(Text key, Iterable<IntWritable> values, Context context) throws IOException, InterruptedException {
			int sum = 0;
 			for (IntWritable val : values) {
				sum += val.get();
 			}
			context.write(key, new IntWritable(sum));
 		}
 	}

	public static void main(String[] args) throws Exception {
 		Configuration conf = new Configuration();
 		Job job = new Job(conf, "repliccount");
 		job.setJarByClass(ReplicCount.class);
		job.setOutputKeyClass(Text.class);
 		job.setOutputValueClass(IntWritable.class);
		job.setMapperClass(Map.class);
		job.setReducerClass(Reduce.class);
		job.setInputFormatClass(TextInputFormat.class);
		job.setOutputFormatClass(TextOutputFormat.class);
		FileInputFormat.addInputPath(job, new Path(args[0]));
		FileOutputFormat.setOutputPath(job, new Path(args[1]));
		job.waitForCompletion(true);
 	}
}
