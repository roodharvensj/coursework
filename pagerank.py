import sys
from operator import add, itemgetter
import time

from pyspark.sql import SparkSession

BETA = 0.80

def parse_edge(line):
    vertices = line.split()
    return (int(vertices[0]), int(vertices[1]))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Missing one or more required arguments", file=sys.stderr)
        print("Usage: pagerank <file> <iterations>", file=sys.stderr)
        sys.exit(-1)

    # Extract command line arguments
    filename = sys.argv[1]
    iterations = int(sys.argv[2])

    # Create spark session using "new" SparkSession API
    spark = SparkSession.builder.appName("PageRank").getOrCreate()
    spark.sparkContext.setLogLevel("WARN") # Reduce logging output

    # Time execution
    begin_time = time.time()

    # Load the file creating an RDD of edge pairs, i.e. (source_vertex, dest_vertex), while
    # filtering out any duplicate edges (using distinct)
    edges = spark.sparkContext.textFile(filename).map(parse_edge).distinct()

    # Determine the number of vertices
    vertices = edges.flatMap(lambda x: x).distinct()
    num_vertices = vertices.count()
    initial= 0.2/num_vertices
    rank= vertices.map(lambda a: (a, initial))
    vertcount= edges.countByKey()
    matrix= edges.map(lambda x: (x[0], x[1], 1/vertcount[x[0]]))
    for i in range(iterations):
        rank_bcast= spark.sparkContext.broadcast(rank.collectAsMap())
        rantrix= matrix.map(lambda x: (x[1],(x[2] * rank_bcast.value.get(x[0]))))
        mank= rantrix.reduceByKey(lambda a, b: a + b)
        rank= mank.map(lambda x: (x[0],(0.8 * x[1]) +initial))
    topTen=rank.takeOrdered(10, lambda b: -b[1] )
    print('\n'.join('{}: {}'.format(*k) for k in topTen))





    # TODO: Implement iterative PageRank algorithm, printing out the top-ten highest ranked nodes in
    # descending order of rank, e.g.
    # 173:    0.01673849720556171
    # 618:    0.016584381121521937
    # ...

    end_time = time.time()
    print(f"Total program time: {(end_time-begin_time):.2f} seconds")

    spark.stop()
