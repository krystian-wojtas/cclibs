#!/usr/bin/awk -f
#
# squeeze.awk
# High Luminosity LHC Squeeze analysis
#
# This script will try to estimate the minimum time needed for the squeeze.
# --------------------------------------------------------------------------

BEGIN {

# Set field separate (FS) to comma to handle csv files.

    FS = ","

# Process arguments as filenames

    if(ARGC < 2)
    {
        print "usage: gawk -f squeeze.awk CSV_Filename..."
        exit -1
    }

    for(file_idx = 1; file_idx < ARGC ; file_idx++)
    {
        n = split(ARGV[file_idx], path, "/")
        split(path[n], filename, ".")

        Squeeze(filename[1], ARGV[file_idx], "results/results-" filename[1] ".csv")
    }

    print "--------------------------------------"

    exit
}

# --------------------------------------------------------------------------

function GetHeading(inputfile, csvfile, expected)
{
    getline < inputfile

    if($1 != expected)
    {
        printf "Error in %s : expected heading '%s' and read '%s'\n", inputfile, expected, $1
        exit -1
    }

    print $0 > csvfile
}

# --------------------------------------------------------------------------

function Squeeze(testname, inputfile, csvfile,  time_limiting)
{
    print "--------------------------------------"

    # Read in header lines

    GetHeading(inputfile, csvfile, "HENRYS")
    split($0, henrys)
    GetHeading(inputfile, csvfile, "OHMS_SER")
    split($0, ohms_ser)
    GetHeading(inputfile, csvfile, "V_POS")
    split($0, v_pos)
    GetHeading(inputfile, csvfile, "V_NEG")
    split($0, v_neg)
    GetHeading(inputfile, csvfile, "I_POS")
    split($0, i_pos)
    GetHeading(inputfile, csvfile, "I_NEG")
    split($0, i_neg)
    GetHeading(inputfile, csvfile, "kmax/nominal")
    split($0, kmax)

    getline < inputfile
    split($0, circuit)

    max_cols = NF

    printf "%s,max_dt,time",$0 > csvfile

    for(c = 2 ; c <= max_cols ; c++)
	{
	    printf ",%s", $c > csvfile
	}

    printf "\n" > csvfile

    for(c = 1 ; c <= max_cols ; c++) printf "," > csvfile

    # Read in dataset

    idx = 0

    while((getline  < inputfile) > 0)
    {
        if(NF != max_cols)
        {
            print "Bad record: ", $1, $2, $3, "..."
            exit -1
        }

        idx++

        for(c = 1 ; c <= NF ; c++) iref[c,idx] = $c
    }

    printf ",0" > csvfile

    for(c = 2 ; c <= max_cols ; c++) printf ",%s", iref[c,idx] > csvfile

    printf "\n" > csvfile

    # Calculate delta times

    sum_dt = 0.0

	ref_idx = 0
	ref_time[ref_idx] = 0

	for(c = 2 ; c <= max_cols ; c++)
	{
		ref_current[c,ref_idx] = iref[c,idx]
	}

	ref_idx++

    while(--idx > 0)
    {
        max_dt = 0.0

        printf "%s", iref[1,idx] > csvfile

        for(c = 2 ; c <= max_cols ; c++)
        {
            dI = iref[c,idx] - iref[c,idx+1]

            if(dI < 0.0)
            {
                dt[c,idx] = dI * henrys[c] / (v_neg[c] - iref[c,idx] * ohms_ser[c])
            }
            else if(dI > 0.0)
            {
                dt[c,idx] = dI * henrys[c] / (v_pos[c] - iref[c,idx] * ohms_ser[c])
            }
            else
            {
                dt[c,idx] = 0.0
            }

            if(dt[c,idx] > max_dt)
            {
                max_dt = dt[c,idx]
                max_c  = c
            }

            printf ",%.3f", dt[c,idx] > csvfile
        }

        time_limiting[max_c] += max_dt
        sum_dt                = sum_dt + max_dt
		ref_time[ref_idx]     = sum_dt

        printf ",%.3f,%.3f", max_dt, sum_dt > csvfile

        for(c = 2 ; c <= max_cols ; c++)
		{
			printf ",%s", iref[c,idx] > csvfile
			ref_current[c,ref_idx] = iref[c,idx]
		}
		ref_idx++

        printf "\n" > csvfile
    }

    # Add line to csvfile giving time that each circuit limited the squeeze

    printf "Limiting time" > csvfile

    for(c = 2 ; c <= max_cols ; c++)
    {
        printf ",%.3f", time_limiting[c] > csvfile

        if(time_limiting[c] > 0)
        {
            printf "%-14s:%14s:%8.3f\n", testname, circuit[c], time_limiting[c]
        }
    }

    printf "%-14s:%14s:%8.3f\n", testname, "Total time", sum_dt

    printf "\nTotal time,%.0f\n", sum_dt > csvfile

    # Generate cctest scripts to simulate each circuit

    for(c = 2 ; c <= max_cols ; c++)
	{
        cctestfile = "cctest/" testname "-" circuit[c]

        printf "# cctest script for %s\n\n", filename[1] "-" circuit[c] > cctestfile

        printf "LOAD HENRYS   %s\n",   henrys[c]   > cctestfile
        printf "LOAD OHMS_SER %s\n",   ohms_ser[c] > cctestfile
        printf "LIMIT I_POS   %s\n",   i_pos[c]    > cctestfile
        printf "LIMIT I_NEG   %s\n",   i_neg[c]    > cctestfile
        printf "LIMIT V_POS   %s\n",   v_pos[c]    > cctestfile
        printf "LIMIT V_NEG   %s\n\n", v_neg[c]    > cctestfile

	    printf "TABLE TIME" > cctestfile
		for(i=0 ; i < ref_idx ; i++) printf " %.3f", ref_time[i] > cctestfile

	    printf "\nTABLE REF" > cctestfile
		for(i=0 ; i < ref_idx ; i++) printf " %s", ref_current[c,i] > cctestfile
	    printf "\n\nRUN\n\n# EOF\n" > cctestfile
     	close(cctestfile)
	}

    close(csvfile)
    close(inputfile)
}
# EOF
