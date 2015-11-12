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
public class WordCount {
public static class Map extends Mapper<LongWritable, Text, Text, IntWritable> {
// класс Map - наследник класса Mapper + параметризованные аргументы
 private final static IntWritable one = new IntWritable(1); //константа с
именем one и начальным значением 1
 private Text word = new Text(); // новая
переменная word типа text
 public void map(LongWritable key, Text value, Context context) throws
IOException, InterruptedException { //ф-ия мап
 // context - объект, описывающий правила обработки входных и выходных
данных (интерфейс доступа к входным и выходным данным)
 String line = value.toString(); // перевод типа переменной
value в строковый тип и запись в переменную line (типа string )
 StringTokenizer tokenizer = new StringTokenizer(line); //разметка
строки на слова, разделителем является пробел
 while (tokenizer.hasMoreTokens()) { //если есть еще часть строки
(т.е. слово), то заходим в цикл и берем еще одно слово
 word.set(tokenizer.nextToken()); // переводим следующее слово в
тип text, которое "понятно" для hadoop
 context.write(word, one); // вывод пары (ключ, значение) на
вход редьюсера
 // context.write(word, new IntWritable(1));
 }
 }
 }
 public static class Reduce extends Reducer<Text, IntWritable, Text,
IntWritable> { //класс Reduce - наследник класса Reducer
 public void reduce(Text key, Iterable<IntWritable> values, Context context)
 throws IOException, InterruptedException { //ф-ия редьюсера со списком
исключений
 int sum = 0;
 for (IntWritable val : values) { // в переменную val записываются все
те "количества из маперов", которые пришли на вход редьюсера из маперов
 sum += val.get(); // суммирование величин кол-ва для
одного слова, val.get - получить значение типа Int из IntWritable
 }
 context.write(key, new IntWritable(sum)); // вывод ответа в заданную
директорию
 }
 }
 public static void main(String[] args) throws Exception {
 Configuration conf = new Configuration(); // конфигурация hadoop
 Job job = new Job(conf, "wordcount");
 job.setJarByClass(WordCount.class); // добавление для того,
чтобы происходила компиляция (способ "задания" запуска байт-кода )
 job.setOutputKeyClass(Text.class); // установка соответствующих
типов на выходе редьюсера
 job.setOutputValueClass(IntWritable.class);
 job.setMapperClass(Map.class);
 job.setReducerClass(Reduce.class);
 job.setInputFormatClass(TextInputFormat.class); // на вход мапера
 job.setOutputFormatClass(TextOutputFormat.class); 
 FileInputFormat.addInputPath(job, new Path(args[0]));
 FileOutputFormat.setOutputPath(job, new Path(args[1]));
 job.waitForCompletion(true); 
 }
}