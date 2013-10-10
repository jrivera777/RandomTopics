#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define DATA_POINTS 100
#define THRESHOLD .10
#define RULES 3

struct Point
{
    double x;
    double y;
    int label;
};

void generate_points(int k, struct Point *pts,
		     int xax, int yax, int xin, int yin)
{
    int i;
    for(i = 0; i < k; i++)
    {
	struct Point p;
	p.x = rand() % 300;
	p.y = rand() % 300;
	pts[i] = p;
    }
}

double distance(struct Point *p1, struct Point *p2)
{
    return sqrt(pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2));
}

void print_data_labels(struct Point *data, int nData)
{
    int i;
    for(i = 0; i < nData; i++)
    {
	printf("{%f, %f} ==> %d\n", data[i].x, data[i].y, data[i].label);
    }
}

struct Point * get_centroid_mean(struct Point *centroid,
				 struct Point *data, int nData)
{
    int i;
    int l_count = 0;
    double averageX = 0;
    double averageY = 0;
    struct Point *new_centroid;
    for(i = 0; i < nData; i++)
    {
	if(data[i].label == centroid->label)
	{
	    l_count++;
	    averageX += data[i].x;
	    averageY += data[i].y;
	}
    }
    new_centroid = malloc(sizeof(struct Point));
    new_centroid->x = averageX / l_count;
    new_centroid->y = averageY / l_count;
    new_centroid->label = centroid->label;
    /* printf("Cluster count for centroid %d was %d\n", centroid->label, l_count); */
    /* printf("AverageX = %f\n", new_centroid->x); */
    return new_centroid;
}

double average(double *d, int cnt)
{
    double avg = 0;
    int i;
    for(i = 0; i < cnt; i++)
	avg += d[i];

    return avg / cnt;
}

int main(int argc, char **argv)
{
    int i, j, iter;
    int k;
    int min_index;
    double min_dist;
    double *delta;
    double *delt_perc;
    struct Point *centroids;
    struct Point *data;
    FILE *file;
    srand(time(NULL));

    printf("K-Means Test\n");
    printf("============\n");
    if (argc > 1)
	k = atoi(argv[1]); //assume first argument is desired k value
    else
	k = 3;

    centroids = malloc(k * sizeof(struct Point));
    data = malloc(DATA_POINTS * sizeof(struct Point));
    delta = malloc(k * sizeof(double));
    delt_perc = malloc(k * sizeof(double));
    generate_points(k, centroids, 0, 0, 0, 0);
    generate_points(DATA_POINTS, data, 0, 0, 0, 0);

    //label centroids
    printf("Initial Centroids\n");
    printf("=================\n");
    for(i = 0; i < k; i++)
    {
	printf("{%f, %f}\n", centroids[i].x, centroids[i].y);
	centroids[i].label = i;
    }


    iter = 0;
    do
    {
	char *gnuplot_commands[RULES];
	gnuplot_commands[1] = "unset key";
	char *plot = malloc(sizeof(char) *(k + 2) * strlen("  'data_clust.dat'  ,") + 20);
	strcat(plot, "plot ");

	//label all sample data from initial centroids
	for(i = 0; i < DATA_POINTS; i++)
	{
	    min_dist = distance(&data[i], &centroids[0]);
	    min_index = 0;
	    int j;
	    for(j = 1; j < k; j++)
	    {
		double dist = distance(&data[i], &centroids[j]);
		if (dist < min_dist)
		{
		    min_dist = dist;
		    min_index = j;
		}
		data[i].label = min_index;
	    }
	}

	//write out data points for this iteration
	for(i = 0; i < k; i++)
	{
	    char fname[strlen("data_clust.dat") + k];
	    sprintf(fname, "data_clust%d.dat", i);
	    file = fopen(fname, "w");
	    strcat(plot, "'");
	    strcat(plot, fname);
	    strcat(plot, "', ");

	    if(file == NULL)
	    {
		perror("Failed to open data file");
		return -1;
	    }
	    for(j = 0; j < DATA_POINTS; j++)
	    {
		if(data[j].label == centroids[i].label)
		    fprintf(file, "%f %f\n", data[j].x, data[j].y);
	    }
	    fclose(file);
	}

	char cenName[strlen("cent.dat") + k];
	sprintf(cenName, "cent_%d.dat", i);
	file = fopen(cenName, "w");
	strcat(plot, "'");
	strcat(plot, cenName);
	strcat(plot, "' ");
	if(file == NULL)
	{
	    perror("Failed to open init_cent.dat");
	    return -1;
	}

	//write out centroids
	for(i = 0; i < k; i++)
	{
	    fprintf(file,"%f %f\n", centroids[i].x, centroids[i].y);
	}
	fclose(file);

	//plot clusters and centroids
	gnuplot_commands[0] = malloc(sizeof(char) * strlen("set title 'K-Means Clustering Iter' ") + iter + 1);
	sprintf(gnuplot_commands[0], "set title 'K-Means Clustering Iter %d'", iter);
	gnuplot_commands[2] = plot;

	//print commands
	printf("\n");
	for(i = 0; i < RULES; i++)
	{
	    printf("%s\n", gnuplot_commands[i]);
	}

	FILE *gnuplotPipe = popen("gnuplot -persistent", "w");
	for(i = 0; i < RULES; i++)
	    fprintf(gnuplotPipe, "%s \n", gnuplot_commands[i]);
	fclose(gnuplotPipe);

	//calculate new centroids based on initial clustering
	printf("New Centroids\n");
	printf("=============\n");
	for(i = 0; i < k; i++)
	{
	    struct Point * mean = get_centroid_mean(&centroids[i],
						    data, DATA_POINTS);
	    double diff = distance(&centroids[i], mean);
	    double change = diff / ((diff + delta[i])/2);
	    delta[i] = diff;
	    delt_perc[i] = change;
	    centroids[i].x = mean->x;
	    centroids[i].y = mean->y;
	    free(mean);
	    printf("{%f, %f}\n", centroids[i].x, centroids[i].y);
	}
	printf("\n");
//	free(plot);
	iter++;
    }
    while(average(delt_perc, k) > THRESHOLD);

    free(centroids);
    free(data);
    free(delta);
    free(delt_perc);

    return 0;
}
