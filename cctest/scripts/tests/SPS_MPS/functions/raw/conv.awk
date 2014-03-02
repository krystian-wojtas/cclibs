# Convert SPS csv function files to cctest compatible functions

BEGIN {

    FS = ","

    if(ARGC < 2)
    {
        printf "Usage: gawk -f %s file file file... 2> run.sh_list\n",ARGV[0]
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
     gsub(/_V1/,"",op)

     printf "Converting %-50s into %s/%s", ip, oppath, op

     of = oppath "/" op

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

     printf "TIME %.3f", time[0] > of

     for(i=1 ; i < n ; i++)
     {
         printf ",%.3f", time[i] > of
     }

     printf "\nREF  %s", ref[0] > of

     for(i=1 ; i < n ; i++)
     {
         printf ",%s", ref[i] > of
     }

     print "\n\n# EOF" > of
     close(of)
     close(ip)

     print " ...  done"

     type = tolower(substr(ip,1,2))

     printf "\"$cctest\" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/%s -m pars/limits -l pars/load_%s -s pars/vs -r pars/amps/reg  > \"$outpath/amps-%s-%s.$file_type\"  &&\n", op, type, toupper(type), toupper(substr(op,4)) >> "/dev/stderr"

}
# EOF
