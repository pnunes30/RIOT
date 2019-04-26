#!/usr/bin/gawk -f
function abs(v) {return v < 0 ? -v : v}
BEGIN {
    prev_line="";
    prev_ms=-1;
    prev_ticks=0;
    diff_ms=-1;
    tot_diff_ms=0;
    numd=0;
    numerr=0;
#    timer_khz_conf=1562.5
#    timer_khz_conf=3125
#    timer_khz_conf=6250
#    timer_khz_conf=12500
#    timer_khz_conf=25000
#    timer_khz_conf=50000
#    timer_khz_conf=16000
    timer_khz_conf=8000
#    exp_diff_ms=(2**32)/timer_khz_conf;
#    err_diff_ms=(1000000/timer_khz_conf)/2;
    samples_cnt=0;
    samples_hz=10;
    exp_diff_ms=(samples_hz*1000);
    curr_c1=0;
    curr_c2=0;
}
/WORKER/ {
    # 2018-04-26 08:59:26,515 - INFO # WORKER => now=4294.830714 (0xfffdea7a ticks), drift=-27 us, jitter=2 us
    # 2018-04-26 08:59:27,154 - INFO # WORKER => now=1.863443 (6+0xd523c897 ticks),  jitter=    4764       o=6,6 e0,0 #0 c=1744000 d=3200 #1 c=467302 d=851 #2 c=467301 d=849
    
    #"date +%s -d \"" datetime "\"" |getline curr_ts;
    # line2=gensub(/^(.+),(.+) - INFO # WORKER => now=(.+) .+$/,"\\1\;\\2;\\3","g",$0)
    # split(line2,arr,";")
    # datetime=arr[1];
    # mstime=arr[2];
    # now=arr[3];
    flg=""
    if ( (match($0, /^(....)-(..)-(..) (..):(..):(..),(...) - INFO # WORKER => now=(.+) \((.+)\+(0x.+) ticks\).+$/, a)) &&
	 (++samples_cnt % samples_hz == 0) )
    {
	curr_time=a[1] " " a[2] " " a[3] " " a[4] " " a[5] " " a[6];
	curr_ts=mktime(curr_time);
	curr_tms=a[7];
	curr_now=a[8];
	curr_period=a[9];
	curr_hex_ticks=a[10];
	if (curr_ts!=-1) {
	    curr_ms= curr_ts*1000+curr_tms;
	    curr_ticks=curr_period*2**32+strtonum(curr_hex_ticks);
	    if (prev_ms>0) {
		diff_ms=curr_ms-prev_ms;
		diff_ticks=(curr_ticks-prev_ticks);
		timer_khz=diff_ticks/diff_ms;
		dev_diff_ms=  (diff_ms-exp_diff_ms)/exp_diff_ms;
		dev_timer_khz=(timer_khz-timer_khz_conf)/timer_khz_conf;
		if ( abs(dev_timer_khz) > 0.0015 ) { # we trigger an error if we deviate of 1/1000 from the timer freq
		    if ( abs(dev_timer_khz) > 0.1 ) { #the config changed
			timer_khz_conf = int((timer_khz+50)/100)*100
		    }
		#if ( abs(dev_diff_ms) > 0.01 ) { # we trigger an error if we deviate of 1%
		    flg="!!!";
		    numerr++;
#		    printf "%s[%s] [%s] diff_ms=[%s] avg[%.1f] dev[%f]\n", flg, curr_ms, numd, diff_ms, avg_diff_ms, dev_diff_ms ;
		    printf " <[%s]\n", prev_line;
		    printf " >[%s]\n", $0;
		}
		else
		{
		    tot_diff_ms+=diff_ms;
		    numd++;
		    avg_diff_ms=tot_diff_ms/numd;
		}		    
	    }
	    if ( (match($0, /^.+ticks\).+o=(.+),(.+) e(.+),(.+) #0/, a)) )
	    {
		curr_o1=a[1];
		curr_o2=a[2];
		curr_e1=a[3];
		curr_e2=a[4];
	    }
	    if ( (match($0, /#0.+#1 c=(.+) d=.+#2 c=(.+) d=.+$/, a)) )
	    {
		curr_d1=a[1]-curr_c1; curr_c1=a[1]
		curr_d2=a[2]-curr_c2; curr_c2=a[2]
	    }
	
	    printf "%s[%s] [%s] diff_ms=[%6d] avg[%7.1f] dev[%5.2f] T[%d+%s] o[%s,%s] e[%s,%s] d[%d,%d] ticks/ms=[%.1f] devkhz[%7.4f]\n", flg, curr_time, samples_cnt, diff_ms, avg_diff_ms, dev_diff_ms, curr_period, curr_hex_ticks, curr_o1, curr_o2, curr_e1, curr_e2, curr_d1*1000/diff_ms, curr_d2*1000/diff_ms, timer_khz, dev_timer_khz;
	    
	    prev_ms=curr_ms;
	    prev_ticks=curr_ticks;
	    prev_line=$0;
	}else{
	    # pyterm timestamps all lines. this should not happen
	    printf "ERROR: mktime conversion failed [%s]\n", $0;
	}
    }else{
	#printf "no match [%s]\n", $0;
    }
}
END {
    printf "END numd=[%d] numerr[%d] diff_ms exp=[%.1f] avg=[%.1f]\n", numd, numerr, exp_diff_ms, avg_diff_ms;
}
