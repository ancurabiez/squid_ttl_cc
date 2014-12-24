squid_ttl_cc
============

Patching squid2.6 untuk mem-mark paket menggunakan geoip.
Sehingga traffic yng berasal dari suatu negara dapat diatur (bandwidth manager)

Marking paket melalui ttl sehingga memanage bandwidth dapat terpisah dengan squid-box

id_lg.c     adalah aplikasi untuk merubah format yang diambil dari site lg_mohonmaaf.com
            menjadi format geoip.
