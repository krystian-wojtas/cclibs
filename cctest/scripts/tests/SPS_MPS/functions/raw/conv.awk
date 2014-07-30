#!/usr/bin/awk -f
# Convert SPS csv function files to cctest compatible functions

BEGIN {

    FS = ","

    if(ARGC < 2)
    {
        printf "Usage: gawk -f %s file file file...\n",ARGV[0]
        exit -1
    }

    for(i = 0 ; i < ARGC ; i++)
    {
        if(split(ARGV[i],filename,".") == 2 && filename[2] == "csv")
        {
            convert(ARGV[i], filename[1], "../amps")
        }
    }
}
#----------------------------------------------------------------
function convert(ip, op, oppath,  n, i, of) 
{
     split(op,path,"/")

     gsub(/_V1/,"",op)

     gsub(/\//,"_",op)

     of = oppath "/" path[1] "/" op

     printf "Converting %-50s into %s", ip, of

     printf "# CCTEST Table for %s\n\n",op > of

# Read and discard the 2 header lines

     getline < ip
     getline < ip

     n = 0;
     delete time
     delete ref

     while((getline < ip) > 0)
     {
         if(NF == 2)
         {
             time[n] = $1 * 0.001
             ref[n]  = $2
             n++
         }
         else
         {
            printf "Error line %n: NF=%u Rec=%s\n", n, NF, $0
         }
     }

     printf "TABLE TIME %.3f", time[0] > of

     for(i=1 ; i < n ; i++)
     {
         printf ",%.3f", time[i] > of
     }

     printf "\nTABLE REF  %s", ref[0] > of

     for(i=1 ; i < n ; i++)
     {
         printf ",%s", ref[i] > of
     }

     print "\n\nRUN\n\n# EOF" > of
     close(of)
     close(ip)

     print " ...  done"
}
# EOF
