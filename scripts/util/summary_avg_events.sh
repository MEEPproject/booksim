# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Author: Ivan Perez

booksim_log_file=$1
opensmart_log_file=$2

echo "BookSim R0: SA-L "
grep "router_0_0.*SA-L" $booksim_log_file | wc -l
echo "BookSim R0: SA-L "
grep "router_0_1.*SA-L" $booksim_log_file | wc -l
echo "BookSim R0: SA-L "
grep "router_0_2.*SA-L" $booksim_log_file | wc -l
echo "BookSim R0: SA-L "
grep "router_0_3.*SA-L" $booksim_log_file | wc -l
echo "BookSim R0: | SA-G |  "
grep "router_0_0.*| SA-G | " $booksim_log_file | wc -l
echo "BookSim R0: | SA-G |  "
grep "router_0_1.*| SA-G | " $booksim_log_file | wc -l
echo "BookSim R0: | SA-G |  "
grep "router_0_2.*| SA-G | " $booksim_log_file | wc -l
echo "BookSim R0: | SA-G |  "
grep "router_0_3.*| SA-G | " $booksim_log_file | wc -l
echo "BookSim R0: ST+LT "
grep "router_0_0.*ST+LT" $booksim_log_file | wc -l
echo "BookSim R0: ST+LT "
grep "router_0_1.*ST+LT" $booksim_log_file | wc -l
echo "BookSim R0: ST+LT "
grep "router_0_2.*ST+LT" $booksim_log_file | wc -l
echo "BookSim R0: ST+LT "
grep "router_0_3.*ST+LT" $booksim_log_file | wc -l
echo "OpenSMART R0: SA-L "
grep "routers_0_0.*SA-L" $opensmart_log_file | wc -l
echo "OpenSMART R0: SA-L "
grep "routers_0_1.*SA-L" $opensmart_log_file | wc -l
echo "OpenSMART R0: SA-L "
grep "routers_0_2.*SA-L" $opensmart_log_file | wc -l
echo "OpenSMART R0: SA-L "
grep "routers_0_3.*SA-L" $opensmart_log_file | wc -l
echo "OpenSMART R0: | SA-G |  "
grep "routers_0_0.*| SA-G | " $opensmart_log_file | wc -l
echo "OpenSMART R0: | SA-G |  "
grep "routers_0_1.*| SA-G | " $opensmart_log_file | wc -l
echo "OpenSMART R0: | SA-G |  "
grep "routers_0_2.*| SA-G | " $opensmart_log_file | wc -l
echo "OpenSMART R0: | SA-G |  "
grep "routers_0_3.*| SA-G | " $opensmart_log_file | wc -l
echo "OpenSMART R0: ST+LT "
grep "routers_0_0.*ST+LT" $opensmart_log_file | wc -l
echo "OpenSMART R0: ST+LT "
grep "routers_0_1.*ST+LT" $opensmart_log_file | wc -l
echo "OpenSMART R0: ST+LT "
grep "routers_0_2.*ST+LT" $opensmart_log_file | wc -l
echo "OpenSMART R0: ST+LT "
grep "routers_0_3.*ST+LT" $opensmart_log_file | wc -l
