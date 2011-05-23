"""
A Python module for running Ferret.

A Python extension module that interfaces with Ferret functionality
and provides methods for Ferret external functions written in Python.

In this module:
    init or start must first be called to initialize Ferret
    resize can be used to resize Ferret's allocated memory block
    run is used to submit individual Ferret commands or enter
            into Ferret's command prompting mode
    get and getdata are used to retrieve (a copy of) a Ferret
            numeric data array
    put and putdata is used to add (a copy of) a numeric data
            array into Ferret
    stop can be used to shutdown Ferret and free the allocated
            memory.

The FERR_* values are the possible values of err_int in the return
values from the run command.  The err_int return value FERR_OK
indicates no errors.

For writing Ferret external functions in Python, see the help message
printed by ferret_pyfunc().  Methods available to these external
functions provided by this module are:
    get_axis_coordinates returns the "world" coordinates for an axis
            of an argument to the external function
    get_axis_box_sizes returns the "box sizes", in "world" coordinate
            units, for an axis of an argument to the external function
    get_axis_box_limits returns the "box limits", in "world" coordinate
            units, for an axis of an argument to the external function
    get_axis_info returns a dictionary of information about the axis
            of an argument to the external function
"""

import sys
import os
import numpy
import numpy.ma
import StringIO
try:
    import cdms2
    import cdtime
except ImportError:
    print >>sys.stderr, "    WARNING: Unable to import cdms2 and/or cdtime; pyferret.get and pyferret.put will fail"
from _pyferret import *


def init(arglist=None, enterferret=True):
    """
    Interprets the traditional Ferret options given in arglist and
    starts pyferret appropriately.  Defines all the standard Ferret
    Python external functions.  If ${HOME}/.ferret exists, that
    script is then executed.

    If '-script' is given with a script filename, this method calls
    the run method with the ferret go command, the script filename,
    and any arguments, and then exits completely (exits python).

    Otherwise, if enterferret is False, this method just returns the
    success return value of the run method: (FERR_OK, '')

    If enterferret is True (unless '-python' is given in arglist)
    this routine calls the run method with no arguments in order to
    enter into Ferret command-line processing.  The value returned
    from call to the run method is then returned.
    """

    ferret_help_message = \
    """

    Usage:  ferret7  [-memsize <N>]  [-batch [<filename>]]  [-gif]  [-nojnl]  [-noverify]
                     [-python]  [-version]  [-help]  [-script <scriptname> [ <scriptarg> ... ]]

       -memsize:   initialize the memory cache size to <N> (default 25.6) megafloats
                   (where 1 float = 4 bytes)
       -batch:     output directly to metafile <filename> (default "metafile.plt")
                   without X-Windows
       -gif:       output to GIF file without X-Windows only with the FRAME command
       -nojnl:     on startup don't open a journal file (can be turned on later with
                   SET MODE JOURNAL)
       -noverify:  on startup turn off verify mode (can be turned on later with
                   SET MODE VERIFY)
       -python:    start at the Python prompt instead of the Ferret prompt
                   (the ferret prompt can be obtained entering 'pyferret.run()')
       -version:   print the Ferret header with version number and quit
       -help:      print this help message and quit
       -script:    execute the script <scriptname> with any arguments specified,
                   and exit (THIS MUST BE SPECIFIED LAST)

    """

    std_pyefs = ( # "stats_helper",
                  "stats_cdf",
                  "stats_isf",
                  "stats_pdf",
                  "stats_pmf",
                  "stats_ppf",
                  "stats_rvs",
                  "stats_sf",
                  "stats_beta_cdf",
                  "stats_beta_isf",
                  "stats_beta_pdf",
                  "stats_beta_ppf",
                  "stats_beta_rvs",
                  "stats_beta_sf",
                  "stats_binom_cdf",
                  "stats_binom_isf",
                  "stats_binom_pmf",
                  "stats_binom_ppf",
                  "stats_binom_rvs",
                  "stats_binom_sf",
                  "stats_cauchy_cdf",
                  "stats_cauchy_isf",
                  "stats_cauchy_pdf",
                  "stats_cauchy_ppf",
                  "stats_cauchy_rvs",
                  "stats_cauchy_sf",
                  "stats_chi_cdf",
                  "stats_chi_isf",
                  "stats_chi_pdf",
                  "stats_chi_ppf",
                  "stats_chi_rvs",
                  "stats_chi_sf",
                  "stats_chi2_cdf",
                  "stats_chi2_isf",
                  "stats_chi2_pdf",
                  "stats_chi2_ppf",
                  "stats_chi2_rvs",
                  "stats_chi2_sf",
                  "stats_expon_cdf",
                  "stats_expon_isf",
                  "stats_expon_pdf",
                  "stats_expon_ppf",
                  "stats_expon_rvs",
                  "stats_expon_sf",
                  "stats_exponweib_cdf",
                  "stats_exponweib_isf",
                  "stats_exponweib_pdf",
                  "stats_exponweib_ppf",
                  "stats_exponweib_rvs",
                  "stats_exponweib_sf",
                  "stats_f_cdf",
                  "stats_f_isf",
                  "stats_f_pdf",
                  "stats_f_ppf",
                  "stats_f_rvs",
                  "stats_f_sf",
                  "stats_gamma_cdf",
                  "stats_gamma_isf",
                  "stats_gamma_pdf",
                  "stats_gamma_ppf",
                  "stats_gamma_rvs",
                  "stats_gamma_sf",
                  "stats_geom_cdf",
                  "stats_geom_isf",
                  "stats_geom_pmf",
                  "stats_geom_ppf",
                  "stats_geom_rvs",
                  "stats_geom_sf",
                  "stats_hypergeom_cdf",
                  "stats_hypergeom_isf",
                  "stats_hypergeom_pmf",
                  "stats_hypergeom_ppf",
                  "stats_hypergeom_rvs",
                  "stats_hypergeom_sf",
                  "stats_invgamma_cdf",
                  "stats_invgamma_isf",
                  "stats_invgamma_pdf",
                  "stats_invgamma_ppf",
                  "stats_invgamma_rvs",
                  "stats_invgamma_sf",
                  "stats_laplace_cdf",
                  "stats_laplace_isf",
                  "stats_laplace_pdf",
                  "stats_laplace_ppf",
                  "stats_laplace_rvs",
                  "stats_laplace_sf",
                  "stats_lognorm_cdf",
                  "stats_lognorm_isf",
                  "stats_lognorm_pdf",
                  "stats_lognorm_ppf",
                  "stats_lognorm_rvs",
                  "stats_lognorm_sf",
                  "stats_nbinom_cdf",
                  "stats_nbinom_isf",
                  "stats_nbinom_pmf",
                  "stats_nbinom_ppf",
                  "stats_nbinom_rvs",
                  "stats_nbinom_sf",
                  "stats_norm_cdf",
                  "stats_norm_isf",
                  "stats_norm_pdf",
                  "stats_norm_ppf",
                  "stats_norm_rvs",
                  "stats_norm_sf",
                  "stats_pareto_cdf",
                  "stats_pareto_isf",
                  "stats_pareto_pdf",
                  "stats_pareto_ppf",
                  "stats_pareto_rvs",
                  "stats_pareto_sf",
                  "stats_poisson_cdf",
                  "stats_poisson_isf",
                  "stats_poisson_pmf",
                  "stats_poisson_ppf",
                  "stats_poisson_rvs",
                  "stats_poisson_sf",
                  "stats_randint_cdf",
                  "stats_randint_isf",
                  "stats_randint_pmf",
                  "stats_randint_ppf",
                  "stats_randint_rvs",
                  "stats_randint_sf",
                  "stats_t_cdf",
                  "stats_t_isf",
                  "stats_t_pdf",
                  "stats_t_ppf",
                  "stats_t_rvs",
                  "stats_t_sf",
                  "stats_uniform_cdf",
                  "stats_uniform_isf",
                  "stats_uniform_pdf",
                  "stats_uniform_ppf",
                  "stats_uniform_rvs",
                  "stats_uniform_sf",
                  "stats_weibull_cdf",
                  "stats_weibull_isf",
                  "stats_weibull_pdf",
                  "stats_weibull_ppf",
                  "stats_weibull_rvs",
                  "stats_weibull_sf",
                )

    my_metaname = None
    my_memsize = 25.6
    my_journal = True
    my_verify = True
    my_enterferret = enterferret
    script = None
    print_help = False
    just_exit = False
    # To be compatible with traditional Ferret command-line options
    # (that are still supported), we need to parse the options by hand.
    try:
        k = 0
        while k < len(arglist):
            opt = arglist[k]
            if opt == "-memsize":
                k += 1
                try:
                    my_memsize = float(arglist[k])
                except:
                    raise ValueError("a positive number must be given for a -memsize value")
                if my_memsize <= 0.0:
                    raise ValueError("a positive number must be given for a -memsize value")
            elif opt == "-batch":
                my_metaname = "metafile.plt"
                k += 1
                # -batch has an optional argument
                try:
                    if arglist[k][0] != '-':
                        my_metaname = arglist[k]
                    else:
                        k -= 1
                except:
                    k -= 1
            elif opt == "-gif":
                my_metaname = ".gif"
            elif opt == "-nojnl":
                my_journal = False
            elif opt == "-noverify":
                my_verify = False
            elif opt == "-python":
                my_enterferret = False
            elif opt == "-version":
                just_exit = True
                break
            elif (opt == "-help") or (opt == "-h") or (opt == "--help"):
                print_help = True
                break
            elif opt == "-script":
                k += 1
                try:
                    script = arglist[k:]
                    if len(script) == 0:
                        raise ValueError("a script filename must be given for the -script value")
                except:
                    raise ValueError("a script filename must be given for the -script value")
                # -script implies -nojnl
                my_journal = False
                break
            else:
                raise ValueError("unrecognized option '%s'" % opt)
            k += 1
    except ValueError, errmsg:
        # print the error message then mark for print the help message
        print >>sys.stderr, "\n%s" % errmsg
        print_help = True
    if print_help:
        # print the help message, then mark for exiting
        print >>sys.stderr, ferret_help_message
        just_exit = True
    if just_exit:
        # print the ferret header then exit completely
        start(journal=False, verify=False, metaname=".gif")
        result = run("exit /program")
        # should not get here
        raise SystemExit
    # start ferret without journaling
    start(memsize=my_memsize, journal=False, verify=my_verify, metaname=my_metaname)
    # define all the Ferret standard Python external functions
    for fname in std_pyefs:
        result = run("DEFINE PYFUNC pyferret.stats.%s" % fname)
    # if journaling desired, now turn on journaling
    if my_journal and (script == None):
        result = run("SET MODE JOURNAL")
    # run the ${HOME}/.ferret script if it exists
    home_val = os.environ.get('HOME')
    if home_val:
        init_script = os.path.join(home_val, '.ferret')
        if os.path.exists(init_script):
            try:
                result = run('go "%s"; exit /topy' % init_script)
            except:
                print >>sys.stderr, " **Error: exception raised in runnning script %s" % init_script
                result = run('exit /program')
                # should not get here
                raise SystemExit
    # if a command-line script is given, run the script and exit completely
    if script != None:
        script_line = " ".join(script)
        try:
            result = run('go "%s"; exit /program' % script_line)
        except:
            print >>sys.stderr, " **Error: exception raised in running script %s" * script_line
        # If exception or if returned early, force shutdown
        result = run('exit /program')
        # should not get here
        raise SystemExit
    # if they don't want to enter ferret, return the success value from run
    if not my_enterferret:
        return (_pyferret.FERR_OK, '')
    # otherwise, go into Ferret command-line processing until "exit /topy" or "exit /program"
    result = run()
    return result


def start(memsize=25.6, journal=True, verify=True, metaname=None):
    """
    Initializes Ferret.  This allocates the initial amount of memory for Ferret
    (from Python-managed memory), opens the journal file, if requested, and sets
    Ferret's verify mode.  If metaname is None or empty, Ferret's graphics are
    are displayed on the X-Windows display; otherwise, this value is used as the
    initial filename for the graphics metafile.  This routine does NOT run any
    user initialization scripts.

    Arguments:
        memsize:  the size, in megafloats (where a "float" is 4 bytes),
                  to allocate for Ferret's memory block
        journal:  turn on Ferret's journal mode?
        verify:   turn on Ferret's verify mode?
        metaname: filename for Ferret graphics, can be None or empty
    Returns:
        True is successful
        False if Ferret has already been started
    Raises:
        ValueError if memsize if not a positive number
        MemoryError if unable to allocate the needed memory
        IOError if unable to open the journal file
    """
    # check memsize
    try:
        flt_memsize = float(memsize)
        if flt_memsize <= 0.0:
            raise ValueError
    except:
        raise ValueError("memsize must be a positive number")
    # check metaname
    if metaname == None:
        str_metaname = ""
    elif not isinstance(metaname, str):
        raise ValueError("metaname must either be None or a string")
    elif metaname.isspace():
        str_metaname = ""
    else:
        str_metaname = metaname
    # the actual call
    return _pyferret._start(flt_memsize, bool(journal), bool(verify), str_metaname)


def resize(memsize):
    """
    Resets the the amount of memory allocated for Ferret from Python-managed memory.

    Arguments:
        memsize: the new size, in megafloats (where a "float" is 4 bytes),
                 for Ferret's memory block
    Returns:
        True if successful - Ferret has the new amount of memory
        False if unsuccessful - Ferret has the previous amount of memory
    Raises:
        ValueError if memsize if not a positive number
        MemoryError if Ferret has not been started or has been stopped
    """
    # check memsize
    try:
        flt_memsize = float(memsize)
        if flt_memsize <= 0.0:
            raise ValueError
    except:
        raise ValueError("memsize must be a positive number")
    # the actual call
    return _pyferret._resize(flt_memsize)


def run(command=None):
    """
    Runs a Ferret command just as if entering a command at the Ferret prompt.

    If the command is not given, is None, or is a blank string, Ferret will
    prompt you for commands until "EXIT /TOPYTHON" is given.  In this case,
    the return tuple will be for the last error, if any, that occurred in
    the sequence of commands given to Ferret.

    Arguments:
        command: the Ferret command to be executed.
    Returns:
        (err_int, err_string)
            err_int: one of the FERR_* data values (FERR_OK if there are no errors)
            err_string: error or warning message (can be empty)
        Error messages normally start with "**ERROR"
        Warning messages normally start with "*** NOTE:"
    Raises:
        ValueError if command is neither None nor a String
        MemoryError if Ferret has not been started or has been stopped
    """
    # check command
    if command == None:
        str_command = ""
    elif not isinstance(command, str):
        raise ValueError("command must either be None or a string")
    elif command.isspace():
        str_command = ""
    else:
        str_command = command
    # the actual call
    retval = _pyferret._run(str_command)
    if (retval[0] == _pyferret._FERR_EXIT_PROGRAM) and (retval[1] == "EXIT"):
        # python -i -c ... intercepts the sys.exit(0) and stays in python.
        # So _pyferret._run(), when is gets the Ferret "exit" command,
        # instead makes a call in C to exit(0) and doesn't return.
        # This was kept here in case it can be made to work.
        stop()
        sys.exit(0)
    return retval


def metastr(datadict):
    """
    Creates a string representation of the metadata in a data dictionary.
    Print this string to show a nicely formatted display of the metadata.

    Arguments:
        datadict: a data dictionary, as returned by the getdata method.
    Returns:
        the string representation of the metadata in datadict.
    Raises:
        TypeError if datadict is not a dictionary
    """
    uc_month = { 1: "JAN", 2:"FEB", 3:"MAR", 4:"APR", 5:"MAY", 6:"JUN",
                 7:"JUL", 8:"AUG", 9:"SEP", 10:"OCT", 11:"NOV", 12:"DEC" }
    if not isinstance(datadict, dict):
        raise TypeError("datadict is not a dictionary")
    # specify an order of output for standard keys, leaving out "data"
    keylist = [ "name", "title", "dset", "data_unit", "missing_value",
                "axis_names", "axis_types", "axis_units", "axis_coords" ]
    # append non-standard keys in alphabetical order
    for key in sorted(datadict.keys()):
        if (key != "data") and (key not in keylist):
            keylist.append(key)
    # create the metadata string using StringIO
    strbuf = StringIO.StringIO()
    for key in keylist:
        try:
            # make sure the key:value pair exists
            val = datadict[key]
            # just in case the key is not a string (for printing)
            keystr = str(key)
            if keystr == "axis_coords":
                print >>strbuf, keystr + ":"
                for (idx, item) in enumerate(val):
                    # add the axis name (which will be present if coordinates
                    # are given) as a label for the axis coordinates
                    itemlabel = "   '" + datadict["axis_names"][idx] + "': "
                    if datadict["axis_types"][idx] == _pyferret.AXISTYPE_TIME:
                        # add a translation of each of the time 6-tuples
                        strlist = [ ]
                        for subitem in item:
                            strlist.append(" %s = %02d-%3s-%04d %02d:%02d:%02d" % \
                                           (str(subitem),
                                                subitem[_pyferret.TIMEARRAY_DAYINDEX],
                                       uc_month[subitem[_pyferret.TIMEARRAY_MONTHINDEX]],
                                                subitem[_pyferret.TIMEARRAY_YEARINDEX],
                                                subitem[_pyferret.TIMEARRAY_HOURINDEX],
						subitem[_pyferret.TIMEARRAY_MINUTEINDEX],
                                                subitem[_pyferret.TIMEARRAY_SECONDINDEX],) )
                        if len(strlist) == 0:
                           strlist.append("[]")
                        else:
                           strlist[0] = "[" + strlist[0][1:]
                           strlist[-1] = strlist[-1] + "]"
                    else:
                        # just print the values of non-time axis coordinates
                        strlist = str(item).split('\n')
                    # adjust the subsequent-line-indent if multiple lines
                    itemstr = itemlabel + strlist[0]
                    indent = " " * len(itemlabel)
                    for addstr in strlist[1:]:
                        itemstr += "\n" + indent + addstr
                    print >>strbuf, itemstr
            elif keystr == "axis_types":
                # add a translation of the axis type number
                valstr = "("
                for (idx, item) in enumerate(val):
                    if idx > 0:
                        valstr += ", "
                    valstr += str(item)
                    if item == _pyferret.AXISTYPE_LONGITUDE:
                        valstr += "=longitude"
                    elif item == _pyferret.AXISTYPE_LATITUDE:
                        valstr += "=latitude"
                    elif item == _pyferret.AXISTYPE_LEVEL:
                        valstr += "=level"
                    elif item == _pyferret.AXISTYPE_TIME:
                        valstr += "=time"
                    elif item == _pyferret.AXISTYPE_CUSTOM:
                        valstr += "=custom"
                    elif item == _pyferret.AXISTYPE_ABSTRACT:
                        valstr += "=abstract"
                    elif item == _pyferret.AXISTYPE_NORMAL:
                        valstr += "=unused"
                valstr += ")"
                print >>strbuf, keystr + ": " + valstr
            elif keystr == "missing_value":
                # print the one value in the missing value array
                print >>strbuf, keystr + ": " + str(val[0])
            else:
                # just print as "key: value", except
                # adjust the subsequent-line-indent if multiple lines
                strlist = str(val).split('\n')
                valstr = strlist[0]
                indent = " " * (len(keystr) + 2)
                for addstr in strlist[1:]:
                    valstr += "\n" + indent + addstr
                print >>strbuf, keystr + ": " + valstr
        except KeyError:
            # known key not present - ignore
            pass
    strval = strbuf.getvalue()
    strbuf.close()
    return strval


def getdata(name, create_mask=True):
    """
    Returns the numeric array and axes information for the data variable
    described in name as a dictionary.

    Arguments:
        name: the name of the numeric data to retrieve
        create_mask: return the numeric data array as a MaskedArray object?
    Returns:
        A dictionary contains the numeric data array and axes information.
        Note that 'name' is not assigned, which is required for the putdata
        method.  The dictionary contains the following key/value pairs:
            'title' : the string passed in the name argument
            'data': the numeric data array.  If create_mask is True, this
                    will be a NumPy float32 MaskedArray object with the
                    masked array properly assigned.  If create_mask is False,
                    this will just be a NumPy float32 ndarray.
            'missing_value': the missing data value.  This will be a NumPy
                    float32 ndarray containing a single value.
            'data_unit': a string describing the unit of the data.
            'axis_types': a list of integer values describing the type of
                    each axis.  Possible values are the following constants
                    defined by the pyferret module:
                        AXISTYPE_LONGITUDE
                        AXISTYPE_LATITUDE
                        AXISTYPE_LEVEL
                        AXISTYPE_TIME
                        AXISTYPE_CUSTOM   (axis units not recognized by Ferret)
                        AXISTYPE_ABSTRACT (axis is unit-less integer values)
                        AXISTYPE_NORMAL   (axis is normal to the data)
            'axis_names': a list of strings giving the name of each axis
            'axis_units': a list of strings giving the unit of each axis.
                    If the axis type is AXISTYPE_TIME, this names the calendar
                    used for the timestamps, as one of the following strings:
                        'CALTYPE_360DAY'
                        'CALTYPE_NOLEAP'
                        'CALTYPE_GREGORIAN'
                        'CALTYPE_JULIAN'
                        'CALTYPE_ALLLEAP'
                        'CALTYPE_NONE'    (calendar not specified)
            'axis_coords': a list of NumPy ndarrays giving the coordinate values
                    for each axis.  If the axis type is neither AXISTYPE_TIME
                    nor AXISTYPE_NORMAL, a NumPy float64 ndarray is given.  If
                    the axis is type AXISTYPE_TIME, a NumPy integer ndarray of
                    shape (N,6) where N is the number of time points.  The six
                    integer values per time point are the day, month, year, hour,
                    minute, and second of the associate calendar for this time
                    axis.  The following constants defined by the pyferret module
                    give the values of these six indices:
                        TIMEARRAY_DAYINDEX
                        TIMEARRAY_MONTHINDEX
                        TIMEARRAY_YEARINDEX
                        TIMEARRAY_HOURINDEX
                        TIMEARRAY_MINUTEINDEX
                        TIMEARRAY_SECONDINDEX
                    (Thus, axis_coords[t, pyferret.TIMEARRAY_YEARINDEX]
                     gives the year of time point t.)
        Note: a relative time axis will be of type AXISTYPE_CUSTOM, with a unit
              indicating the starting point, such as 'days since 01-JAN-2000'
    Raises:
        ValueError if the data name is invalid
        MemoryError if Ferret has not been started or has been stopped
    See also:
        get
    """
    # lists of units (in uppercase) for checking if a custom axis is actual a longitude axis
    UC_LONGITUDE_UNITS = [ "DEG E", "DEG_E", "DEG EAST", "DEG_EAST",
                           "DEGREES E", "DEGREES_E", "DEGREES EAST", "DEGREES_EAST",
                           "DEG W", "DEG_W", "DEG WEST", "DEG_WEST",
                           "DEGREES W", "DEGREES_W", "DEGREES WEST", "DEGREES_WEST" ]
    # lists of units (in uppercase) for checking if a custom axis is actual a latitude axis
    UC_LATITUDE_UNITS  = [ "DEG N", "DEG_N", "DEG NORTH", "DEG_NORTH",
                           "DEGREES N", "DEGREES_N", "DEGREES NORTH", "DEGREES_NORTH",
                           "DEG S", "DEG_S", "DEG SOUTH", "DEG_SOUTH",
                           "DEGREES S", "DEGREES_S", "DEGREES SOUTH", "DEGREES_SOUTH" ]
    # check name
    if not isinstance(name, str):
        raise ValueError("name must be a string")
    elif name.isspace():
        raise ValueError("name cannot be an empty string")
    # get the data and related information from Ferret
    vals = _pyferret._get(name)
    # break apart the tuple to simplify (returning a dictionary would have been better)
    data = vals[0]
    bdfs = vals[1]
    data_unit = vals[2]
    axis_types = vals[3]
    axis_names = vals[4]
    axis_units = vals[5]
    axis_coords = vals[6]
    # A custom axis could be standard axis that is not in Ferret's expected order,
    # so check the units
    for k in xrange(_pyferret._MAX_FERRET_NDIM):
        if axis_types[k] == _pyferret.AXISTYPE_CUSTOM:
            uc_units = axis_units[k].upper()
            if uc_units in UC_LONGITUDE_UNITS:
                axis_types[k] = _pyferret.AXISTYPE_LONGITUDE
            elif uc_units in UC_LATITUDE_UNITS:
                axis_types[k] = _pyferret.AXISTYPE_LATITUDE
    # _pyferret._get returns a copy of the data, so no need to force a copy
    if create_mask:
        if numpy.isnan(bdfs[0]):
            # NaN comparisons always return False, even to another NaN
            datavar = numpy.ma.array(data, fill_value=bdfs[0], mask=numpy.isnan(data))
        else:
            # since values in data and bdfs[0] are all float32 values assigned by Ferret,
            # using equality should work correctly
            datavar = numpy.ma.array(data, fill_value=bdfs[0], mask=( data == bdfs[0] ))
    else:
        datavar = data
    return { "title": name, "data":datavar, "missing_value":bdfs, "data_unit":data_unit,
             "axis_types":axis_types, "axis_names":axis_names, "axis_units":axis_units,
             "axis_coords":axis_coords }


def get(name, create_mask=True):
    """
    Returns the numeric data array described in name as a TransientVariable object.

    Arguments:
        name: the name of the numeric data array to retrieve
        create_mask: create the mask for the TransientVariable object?
    Returns:
        A cdms2 TransientVariable object (cdms2.tvariable) containing the
        numeric data.  The data, axes, and missing value will be assigned.
        If create_mask is True (or not given), the mask attribute will be
        assigned using the missing value.
    Raises:
        ValueError if the data name is invalid
        MemoryError if Ferret has not been started or has been stopped
    See also:
        getdata
    """
    # lists of units (in lowercase) for checking if a custom axis can be represented by a cdtime.reltime
    # the unit must be followed by "since" and something else
    LC_TIME_UNITS = [ "s", "sec", "secs", "second", "seconds",
                      "mn", "min", "mins", "minute", "minutes",
                      "hr", "hour", "hours",
                      "dy", "day", "days",
                      "mo", "month", "months",
                      "season", "seasons",
                      "yr", "year", "years" ]
    lc_month_nums = { "jan":1, "feb":2, "mar":3, "apr":4, "may":5, "jun":6,
                      "jul":7, "aug":8, "sep":9, "oct":10, "nov":11, "dec":12 }
    # get the data and related information from Ferret,
    # building on what was done in getdata
    data_dict = getdata(name, create_mask)
    data = data_dict["data"]
    bdfs = data_dict["missing_value"]
    data_unit = data_dict["data_unit"]
    axis_types = data_dict["axis_types"]
    axis_names = data_dict["axis_names"]
    axis_units = data_dict["axis_units"]
    axis_coords = data_dict["axis_coords"]
    # create the axis list for this variable
    var_axes = [ ]
    for k in xrange(_pyferret._MAX_FERRET_NDIM):
        if axis_types[k] == _pyferret.AXISTYPE_LONGITUDE:
            newaxis = cdms2.createAxis(axis_coords[k], id=axis_names[k])
            newaxis.units = axis_units[k]
            newaxis.designateLongitude()
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_LATITUDE:
            newaxis = cdms2.createAxis(axis_coords[k], id=axis_names[k])
            newaxis.units = axis_units[k]
            newaxis.designateLatitude()
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_LEVEL:
            newaxis = cdms2.createAxis(axis_coords[k], id=axis_names[k])
            newaxis.units = axis_units[k]
            newaxis.designateLevel()
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_TIME:
            # create the time axis from cdtime.comptime (component time) objects
            time_coords = axis_coords[k]
            timevals = [ ]
            for t in xrange(time_coords.shape[0]):
                day = time_coords[t, _pyferret.TIMEARRAY_DAYINDEX]
                month = time_coords[t, _pyferret.TIMEARRAY_MONTHINDEX]
                year = time_coords[t, _pyferret.TIMEARRAY_YEARINDEX]
                hour = time_coords[t, _pyferret.TIMEARRAY_HOURINDEX]
                minute = time_coords[t, _pyferret.TIMEARRAY_MINUTEINDEX]
                second = time_coords[t, _pyferret.TIMEARRAY_SECONDINDEX]
                timevals.append( cdtime.comptime(year,month,day,hour,minute,second) )
            newaxis = cdms2.createAxis(timevals, id=axis_names[k])
            # designate the calendar
            if axis_units[k] == "CALTYPE_360DAY":
                calendar_type = cdtime.Calendar360
            elif axis_units[k] == "CALTYPE_NOLEAP":
                calendar_type = cdtime.NoLeapCalendar
            elif axis_units[k] == "CALTYPE_GREGORIAN":
                calendar_type = cdtime.GregorianCalendar
            elif axis_units[k] == "CALTYPE_JULIAN":
                calendar_type = cdtime.JulianCalendar
            else:
                if axis_units[k] == "CALTYPE_ALLLEAP":
                    raise ValueError("The all-leap calendar not support by cdms2")
                if axis_units[k] == "CALTYPE_NONE":
                    raise ValueError("Undesignated calendar not support by cdms2")
                raise RuntimeError("Unexpected calendar type of %s" % axis_units[k])
            newaxis.designateTime(calendar=calendar_type)
            # and finally append it to the axis list
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_CUSTOM:
            # Check a custom axis for relative time units.  Note that getdata has
            # already dealt with longitude or latitude not in Ferret's standard position.
            lc_vals = axis_units[k].lower().split()
            if (len(lc_vals) > 2) and (lc_vals[1] == "since") and (lc_vals[0] in LC_TIME_UNITS):
                # (unit) since (start_date)
                datevals = lc_vals[2].split("-")
                try:
                    # try to convert dd-mon-yyyy Ferret-style start_date to yyyy-mm-dd
                    day_num = int(datevals[0])
                    mon_num = lc_month_nums[datevals[1]]
                    year_num = int(datevals[2])
                    lc_vals[2] = "%04d-%02d-%02d" % (year_num, mon_num, day_num)
                    relunit = " ".join(lc_vals)
                except (IndexError, KeyError, ValueError):
                    # use the relative time unit as given
                    relunit = " ".join(lc_vals)
                timevals = [ ]
                for t in xrange(axis_coords[k].shape[0]):
                    dtval = cdtime.reltime(axis_coords[k][t], relunit)
                    timevals.append(dtval)
                newaxis = cdms2.createAxis(timevals, id=axis_names[k])
                newaxis.designateTime()
            else:
                newaxis = cdms2.createAxis(axis_coords[k], id=axis_names[k])
                newaxis.units = axis_units[k]
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_ABSTRACT:
            newaxis = cdms2.createAxis(axis_coords[k], id=axis_names[k])
            var_axes.append(newaxis)
        elif axis_types[k] == _pyferret.AXISTYPE_NORMAL:
            var_axes.append(None)
        else:
            raise RuntimeError("Unexpected axis type of %d" % axis_types[k])
    # getdata returns a copy of the data, thus createVariable does not
    # need to force a copy.  The mask, if request, was created by getdata.
    datavar = cdms2.createVariable(data, fill_value=bdfs[0], axes=var_axes,
                                   attributes={"name":name, "units":data_unit})
    return datavar


def put(datavar, axis_pos=None):
    """
    Creates a Ferret data variable with a copy of the data given in the
    AbstractVariable object datavar.

    Arguments:

        datavar:  a cdms2 AbstractVariable describing the data variable
                  to be created in Ferret.  Any masked values in the data
                  of datavar will be set to the missing value for datavar
                  before extracting the data as 32-bit floating-point
                  values for Ferret.  In addition to the data and axes
                  described in datavar, the following attributes are used:
                      id: the code name for the variable in Ferret (eg,
                          'SST').  This name must be present and, ideally,
                          should not contain spaces, quotes, or algebraic
                          symbols.
                      name: the title name of the variable in Ferret (eg,
                          'Sea Surface Temperature').  If not present, the
                          value of the id attribute is used.
                      units: the unit name for the data.  If not present,
                          no units are associated with the data.
                      dset: Ferret dataset name or number to be associated
                          with this new variable.  If not given or blank,
                          the variable is associated with the current dataset.
                          If None or 'None', the variable is not associated
                          with any dataset.

        axis_pos: a four-tuple giving the Ferret positions for each axis in
                  datavar.  If the axes in datavar are in (time, level, lat.,
                  long.) order, the tuple (T_AXIS, Z_AXIS, Y_AXIS, X_AXIS)
                  should be used for proper axis handling in Ferret.  If not
                  given (or None), the first longitude axis will be made the
                  X_AXIS, the first latitude axis will be made the Y_AXIS,
                  the first level axis will be made the Z_AXIS, the first
                  time axis will be made the T_AXIS, and any remaining axes
                  are then filled into the remaining unassigned positions.

    Returns:
        None

    Raises:
        AttributeError: if datavar is missing a required method or attribute
        MemoryError:    if Ferret has not been started or has been stopped
        ValueError:     if there is a problem with the contents of the arguments

    See also:
        putdata
    """
    #
    # code name for the Ferret variable
    codename = datavar.id.strip()
    if codename == "":
        raise ValueError("The id attribute must be a non-blank string")
    #
    # title name for the Ferret variable
    try:
        titlename = datavar.name.strip()
    except AttributeError:
        titlename = codename
    #
    # units for the data
    try:
        data_unit = datavar.units.strip()
    except AttributeError:
        data_unit = ""
    #
    # missing data value
    missingval = datavar.getMissing()
    #
    # Ferret dataset for the variable; None / 'None' is different from blank / empty
    try:
        dset_str = str(datavar.dset).strip()
    except AttributeError:
        dset_str = ""
    #
    # get the list of axes and initialize the axis information lists
    axis_list = datavar.getAxisList()
    if len(axis_list) > _pyferret._MAX_FERRET_NDIM:
        raise ValueError("More than %d axes is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
    axis_types = [ _pyferret.AXISTYPE_NORMAL ] * _pyferret._MAX_FERRET_NDIM
    axis_names = [ "" ] * _pyferret._MAX_FERRET_NDIM
    axis_units = [ "" ] * _pyferret._MAX_FERRET_NDIM
    axis_coords = [ None ] * _pyferret._MAX_FERRET_NDIM
    for k in xrange(len(axis_list)):
        #
        # get the information for this axis
        axis = axis_list[k]
        axis_names[k] = axis.id.strip()
        try:
            axis_units[k] = axis.units.strip()
        except AttributeError:
            axis_units[k] = ""
        axis_data = axis.getData()
        #
        # assign the axis information
        if axis.isLongitude():
            axis_types[k] = _pyferret.AXISTYPE_LONGITUDE
            axis_coords[k] = axis_data
        elif axis.isLatitude():
            axis_types[k] = _pyferret.AXISTYPE_LATITUDE
            axis_coords[k] = axis_data
        elif axis.isLevel():
            axis_types[k] = _pyferret.AXISTYPE_LEVEL
            axis_coords[k] = axis_data
        elif axis.isTime():
            #
            # try to create a time axis reading the values as cdtime comptime objects
            try:
                time_coords = numpy.empty((len(axis_data),6), dtype=numpy.int32, order="C")
                for t in xrange(len(axis_data)):
                    tval = axis_data[t]
                    time_coords[t, _pyferret.TIMEARRAY_DAYINDEX] = tval.day
                    time_coords[t, _pyferret.TIMEARRAY_MONTHINDEX] = tval.month
                    time_coords[t, _pyferret.TIMEARRAY_YEARINDEX] = tval.year
                    time_coords[t, _pyferret.TIMEARRAY_HOURINDEX] = tval.hour
                    time_coords[t, _pyferret.TIMEARRAY_MINUTEINDEX] = tval.minute
                    time_coords[t, _pyferret.TIMEARRAY_SECONDINDEX] = int(tval.second)
                axis_types[k] = _pyferret.AXISTYPE_TIME
                axis_coords[k] = time_coords
                # assign the axis_units value to the CALTYPE_ calendar type string
                calendar_type = axis.getCalendar()
                if calendar_type == cdtime.Calendar360:
                    axis_units[k] = "CALTYPE_360DAY"
                elif calendar_type == cdtime.NoLeapCalendar:
                    axis_units[k] = "CALTYPE_NOLEAP"
                elif calendar_type == cdtime.GregorianCalendar:
                    axis_units[k] = "CALTYPE_GREGORIAN"
                elif calendar_type == cdtime.JulianCalendar:
                    axis_units[k] = "CALTYPE_JULIAN"
                else:
                    if calendar_type == cdtime.MixedCalendar:
                        raise ValueError("The cdtime.MixedCalendar not support by pyferret")
                    raise ValueError("Unknown cdtime calendar %s" % str(calendar_type))
            except AttributeError:
                axis_types[k] = _pyferret.AXISTYPE_CUSTOM
            #
            # if not comptime objects, assume reltime objects - create as a custom axis
            if axis_types[k] == _pyferret.AXISTYPE_CUSTOM:
                time_coords = numpy.empty((len(axis_data),), dtype=numpy.float64)
                for t in xrange(len(axis_data)):
                    time_coords[t] = axis_data[t].value
                axis_coords[k] = timecoords
                # assign axis_units as the reltime units - makes sure all are the same
                axis_units[k] = axis_data[0].units
                for t in xrange(1, len(axis_data)):
                    if axis_data[t].units != axis_units[k]:
                        raise ValueError("Relative time axis does not have a consistent start point")
        #
        # cdms2 will create an axis if None (normal axis) was given, so create a
        # custom or abstract axis only if it does not look like a cdms2-created axis
        elif not ( (axis_units[k] == "") and (len(axis_data) == 1) and (axis_data[0] == 0.0) and \
                   (axis_data.dtype == numpy.dtype('float64')) and \
                   axis_names[k].startswith("axis_") and axis_names[k][5:].isdigit() ):
            axis_types[k] = _pyferret.AXISTYPE_CUSTOM
            axis_coords[k] = axis_data
            # if a unitless integer value axis, it is abstract instead of custom
            if axis_units[k] == "":
                axis_int_vals = numpy.array(axis_data, dtype=int)
                if numpy.allclose(axis_data, axis_int_vals):
                    axis_types[k] = _pyferret.AXISTYPE_ABSTRACT
    #
    # datavar is an embelished masked array
    datavar_dict = { 'name': codename, 'title': titlename, 'dset': dset_str, 'data': datavar,
                     'missing_vaue': missingval, 'data_unit': data_unit, 'axis_types': axis_types,
                     'axis_names': axis_names, 'axis_units': axis_units, 'axis_coords': axis_coords }
    #
    # use putdata to set defaults, rearrange axes, and add copies
    # of data in the appropriate format to Ferret
    putdata(datavar_dict, axis_pos)
    return None


def putdata(datavar_dict, axis_pos=None):
    """
    Creates a Ferret data variable with a copy of the data given in the dictionary
    datavar_dict, reordering the data and axes according to tuple axis_pos.

    Arguments:

        datavar_dict: a dictionary with the following keys and associated values:
            'name': the code name for the variable in Ferret (eg, 'SST').
                    Must be given.
            'title': the title name for the variable in Ferret (eg, 'Sea Surface
                    Temperature').  If not given, the value of 'name' is used.
            'dset' : the Ferret dataset name or number to associate with this new data
                    variable.  If blank or not given, the current dataset is used.  If
                    None or 'None', no dataset will be associated with the new variable.
            'data': a NumPy numeric ndarray or masked array.  The data will be saved
                    in Ferret as a 32-bit floating-point values.  Must be given.
            'missing_value': the missing data value.  This will be saved in Ferret as
                    a 32-bit floating-point value.  If not given, Ferret's default
                    missing value (-1.0E34) will be used.
            'data_unit': a string describing the unit of the data.  If not given, no
                    unit will be assigned.
            'axis_types': a list of integer values describing the type of each axis.
                    Possible values are the following constants defined by the pyferret
                    module:
                        AXISTYPE_LONGITUDE
                        AXISTYPE_LATITUDE
                        AXISTYPE_LEVEL
                        AXISTYPE_TIME
                        AXISTYPE_CUSTOM   (axis units not interpreted by Ferret)
                        AXISTYPE_ABSTRACT (axis is unit-less integer values)
                        AXISTYPE_NORMAL   (axis is normal to the data)
                    If not given, AXISTYPE_ABSTRACT will be used if the data array
                    has data for that axis (shape element greater than one); otherwise,
                    AXISTYPE_NORMAL will be used.
            'axis_names': a list of strings giving the name of each axis.  If not given,
                    Ferret will generate names if needed.
            'axis_units': a list of strings giving the unit of each axis.
                    If the axis type is AXISTYPE_TIME, this names the calendar
                    used for the timestamps, as one of the following strings:
                        'CALTYPE_360DAY'
                        'CALTYPE_NOLEAP'
                        'CALTYPE_GREGORIAN'
                        'CALTYPE_JULIAN'
                        'CALTYPE_ALLLEAP'
                        'CALTYPE_NONE'    (calendar not specified)
                    If not given, 'DEGREES_E' will be used for AXISTYPE_LONGITUDE,
                    'DEGREES_N' for AXISTYPE_LATITUDE, 'CALTYPE_GREGORIAN' for
                    AXISTYPE_TIME, and no units will be given for other axis types.
            'axis_coords': a list of arrays of coordinates for each axis.
                    If the axis type is neither AXISTYPE_TIME nor AXISTYPE_NORMAL,
                    a one-dimensional numeric list or ndarray should be given (the
                    values will be stored as floating-point values).
                    If the axis is type AXISTYPE_TIME, a two-dimension list or ndarray
                    with shape (N,6), where N is the number of time points, should be
                    given.  The six integer values per time point are the day, month,
                    year, hour, minute, and second of the associate calendar for this
                    time axis.  The following constants defined by the pyferret module
                    give the values of these six indices:
                        TIMEARRAY_DAYINDEX
                        TIMEARRAY_MONTHINDEX
                        TIMEARRAY_YEARINDEX
                        TIMEARRAY_HOURINDEX
                        TIMEARRAY_MINUTEINDEX
                        TIMEARRAY_SECONDINDEX
                    (Thus, axis_coords[t, pyferret.TIMEARRAY_YEARINDEX] gives the year of
                     time point t.)
                    An array of coordinates must be given if the axis does not have a type
                    of AXISTYPE_NORMAL or AXISTYPE_ABSTRACT (or if axis types are not given).
            Note: a relative time axis should be given as type AXISTYPE_CUSTOM, with a
                  unit indicating the starting point, such as 'days since 01-JAN-2000'

        axis_pos: a four-tuple giving the Ferret positions for each axis in datavar.
            If the axes in datavar are in (time, level, lat., long.) order, the tuple
            (T_AXIS, Z_AXIS, Y_AXIS, X_AXIS) should be used for proper axis handling in
            Ferret.  If not given (or None), the first longitude axis will be made the
            X_AXIS, the first latitude axis will be made the Y_AXIS, the first level axis
            will be made the Z_AXIS, the first time axis will be made the T_AXIS, and
            any remaining axes are then filled into the remaining unassigned positions.

    Returns:
        None

    Raises:
        KeyError: if datavar_dict is missing a required key
        MemoryError: if Ferret has not been started or has been stopped
        ValueError:  if there is a problem with the value of a key

    See also:
        put
    """
    #
    # code name for the variable
    codename = datavar_dict.get('name', '')
    if codename != None:
        codename = str(codename).strip()
    if not codename:
        raise ValueError("The value of 'name' must be a non-blank string")
    #
    # title for the variable
    titlename = str(datavar_dict.get('title', codename)).strip()
    #
    # Ferret dataset for the variable; None gets turned into the string 'None'
    dset_str = str(datavar_dict.get('dset', '')).strip()
    #
    # value for missing data
    missingval = float(datavar_dict.get('missing_value', -1.0E34))
    #
    # data units
    data_unit = str(datavar_dict.get('data_unit', '')).strip()
    #
    # axis types
    axis_types = [ _pyferret.AXISTYPE_NORMAL ] * _pyferret._MAX_FERRET_NDIM
    given_axis_types = datavar_dict.get('axis_types', None)
    if given_axis_types:
        if len(given_axis_types) > _pyferret._MAX_FERRET_NDIM:
            raise ValueError("More than %d axes (in the types) is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
        for k in xrange(len(given_axis_types)):
            axis_types[k] = given_axis_types[k]
    #
    # axis names
    axis_names = [ "" ] * _pyferret._MAX_FERRET_NDIM
    given_axis_names = datavar_dict.get('axis_names', None)
    if given_axis_names:
        if len(given_axis_names) > _pyferret._MAX_FERRET_NDIM:
            raise ValueError("More than %d axes (in the names) is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
        for k in xrange(len(given_axis_names)):
            axis_names[k] = given_axis_names[k]
    #
    # axis units
    axis_units = [ "" ] * _pyferret._MAX_FERRET_NDIM
    given_axis_units = datavar_dict.get('axis_units', None)
    if given_axis_units:
        if len(given_axis_units) > _pyferret._MAX_FERRET_NDIM:
            raise ValueError("More than %d axes (in the units) is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
        for k in xrange(len(given_axis_units)):
            axis_units[k] = given_axis_units[k]
    # axis coordinates
    axis_coords = [ None ] * _pyferret._MAX_FERRET_NDIM
    given_axis_coords = datavar_dict.get('axis_coords', None)
    if given_axis_coords:
        if len(given_axis_coords) > _pyferret._MAX_FERRET_NDIM:
            raise ValueError("More than %d axes (in the coordinates) is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
        for k in xrange(len(given_axis_units)):
            axis_coords[k] = given_axis_coords[k]
    #
    # data array
    datavar = datavar_dict['data']
    #
    # For any axis with data (shape > 1), if AXISTYPE_NORMAL (presumably from not being specified),
    # change to AXISTYPE_ABSTRACT.  Note that a shape == 1 could either be normal or a singleton axis.
    try:
        shape = datavar.shape
        if len(shape) > _pyferret._MAX_FERRET_NDIM:
            raise ValueError("More than %d axes (in the data) is not supported in Ferret at this time" % _pyferret._MAX_FERRET_NDIM)
        for k in xrange(len(shape)):
            if (shape[k] > 1) and (axis_types[k] == _pyferret.AXISTYPE_NORMAL):
                axis_types[k] = _pyferret.AXISTYPE_ABSTRACT
    except AttributeError:
        raise ValueError("The value of 'data' must be a NumPy ndarray (or derived from an ndarray)")
    #
    # assign any defaults on the axis information not given,
    # and make a copy of the axis coordinates (to ensure they are well-behaved)
    for k in xrange(_pyferret._MAX_FERRET_NDIM):
        if axis_types[k] == _pyferret.AXISTYPE_LONGITUDE:
            if not axis_units[k]:
                axis_units[k] = "DEGREES_E"
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.float64, copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_LATITUDE:
            if not axis_units[k]:
                axis_units[k] = "DEGREES_N"
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.float64, copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_LEVEL:
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.float64, copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_TIME:
            if not axis_units[k]:
                axis_units[k] = "CALTYPE_GREGORIAN"
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.int32, order='C', copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
            if axis_coords[k].shape[1] != 6:
                raise ValueError("number of components (second index) for time axis %d is not 6" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_CUSTOM:
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.float64, copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_ABSTRACT:
            axis_coords[k] = numpy.array(axis_coords[k], dtype=numpy.float64, copy=1)
            if axis_coords[k].shape[0] != shape[k]:
                raise ValueError("number of coordinates for axis %d does not match the number of data points" % (k+1))
        elif axis_types[k] == _pyferret.AXISTYPE_NORMAL:
            axis_coords[k] = None
        else:
            raise RuntimeError("Unexpected axis_type of %d" % axis_types[k])
    #
    # figure out the desired axis order
    if axis_pos != None:
        # start with the positions provided by the user
        ferr_axis = list(axis_pos)
        if len(ferr_axis) < len(shape):
            raise ValueError("axis_pos, if given, must provide a position for each axis in the data")
        # append undefined axes positions, which were initialized to AXISTYPE_NORMAL
        if not _pyferret.X_AXIS in ferr_axis:
            ferr_axis.append(_pyferret.X_AXIS)
        if not _pyferret.Y_AXIS in ferr_axis:
            ferr_axis.append(_pyferret.Y_AXIS)
        if not _pyferret.Z_AXIS in ferr_axis:
            ferr_axis.append(_pyferret.Z_AXIS)
        if not _pyferret.T_AXIS in ferr_axis:
            ferr_axis.append(_pyferret.T_AXIS)
        # intentionally left as 4 (instead of _MAX_FERRET_NDIM) since new axes will need to be appended
        if len(ferr_axis) != 4:
            raise ValueError("axis_pos can contain at most one of each of the pyferret integer values X_AXIS, Y_AXIS, Z_AXIS, or T_AXIS")
    else:
        ferr_axis = [ -1 ] * _pyferret._MAX_FERRET_NDIM
        # assign positions of longitude/latitude/level/time
        for k in xrange(len(axis_types)):
            if axis_types[k] == _pyferret.AXISTYPE_LONGITUDE:
                if not _pyferret.X_AXIS in ferr_axis:
                    ferr_axis[k] = _pyferret.X_AXIS
            elif axis_types[k] == _pyferret.AXISTYPE_LATITUDE:
                if not _pyferret.Y_AXIS in ferr_axis:
                    ferr_axis[k] = _pyferret.Y_AXIS
            elif axis_types[k] == _pyferret.AXISTYPE_LEVEL:
                if not _pyferret.Z_AXIS in ferr_axis:
                    ferr_axis[k] = _pyferret.Z_AXIS
            elif axis_types[k] == _pyferret.AXISTYPE_TIME:
                if not _pyferret.T_AXIS in ferr_axis:
                    ferr_axis[k] = _pyferret.T_AXIS
        # fill in other axes types in unused positions
        if not _pyferret.X_AXIS in ferr_axis:
            ferr_axis[ferr_axis.index(-1)] = _pyferret.X_AXIS
        if not _pyferret.Y_AXIS in ferr_axis:
            ferr_axis[ferr_axis.index(-1)] = _pyferret.Y_AXIS
        if not _pyferret.Z_AXIS in ferr_axis:
            ferr_axis[ferr_axis.index(-1)] = _pyferret.Z_AXIS
        if not _pyferret.T_AXIS in ferr_axis:
            ferr_axis[ferr_axis.index(-1)] = _pyferret.T_AXIS
        try:
            ferr_axis.index(-1)
            raise RuntimeError("Unexpected undefined axis position (_MAX_FERRET_NDIM increased?) in ferr_axis " + str(ferr_axis))
        except ValueError:
            # expected result
            pass
    #
    # get the missing data value as a 32-bit float
    bdfval = numpy.array(missingval, dtype=numpy.float32)
    #
    # if a masked array, make sure the masked values are set
    # to the missing value, and get the ndarray underneath
    try:
        if numpy.any(datavar.mask):
            datavar.data[datavar.mask] = bdfval
        data = datavar.data
    except AttributeError:
        data = datavar
    #
    # get the data as an ndarray of _MAX_FERRET_NDIM dimensions
    # adding new axes still reference the original data array - just creates new shape and stride objects
    for k in xrange(len(shape), _pyferret._MAX_FERRET_NDIM):
        data = data[..., numpy.newaxis]
    #
    # swap data axes and axis information to give (X_AXIS, Y_AXIS, Z_AXIS, T_AXIS) axes
    # swapping axes still reference the original data array - just creates new shape and stride objects
    k = ferr_axis.index(_pyferret.X_AXIS)
    if k != 0:
        data = data.swapaxes(0, k)
        ferr_axis[0], ferr_axis[k] = ferr_axis[k], ferr_axis[0]
        axis_types[0], axis_types[k] = axis_types[k], axis_types[0]
        axis_names[0], axis_names[k] = axis_names[k], axis_names[0]
        axis_units[0], axis_units[k] = axis_units[k], axis_units[0]
        axis_coords[0], axis_coords[k] = axis_coords[k], axis_coords[0]
    k = ferr_axis.index(_pyferret.Y_AXIS)
    if k != 1:
        data = data.swapaxes(1, k)
        ferr_axis[1], ferr_axis[k] = ferr_axis[k], ferr_axis[1]
        axis_types[1], axis_types[k] = axis_types[k], axis_types[1]
        axis_names[1], axis_names[k] = axis_names[k], axis_names[1]
        axis_units[1], axis_units[k] = axis_units[k], axis_units[1]
        axis_coords[1], axis_coords[k] = axis_coords[k], axis_coords[1]
    k = ferr_axis.index(_pyferret.Z_AXIS)
    if k != 2:
        data = data.swapaxes(2, k)
        ferr_axis[2], ferr_axis[k] = ferr_axis[k], ferr_axis[2]
        axis_types[2], axis_types[k] = axis_types[k], axis_types[2]
        axis_names[2], axis_names[k] = axis_names[k], axis_names[2]
        axis_units[2], axis_units[k] = axis_units[k], axis_units[2]
        axis_coords[2], axis_coords[k] = axis_coords[k], axis_coords[2]
    # T_AXIS must now be ferr_axis[3]
    # assumes _MAX_FERRET_NDIM == 4; extend the logic if axes are added
    # would rather not assume X_AXIS == 0, Y_AXIS == 1, Z_AXIS == 2, T_AXIS == 3
    #
    # now make a copy of the data as (contiguous) 32-bit floats in Fortran order
    fdata = numpy.array(data, dtype=numpy.float32, order='F', copy=1)
    #
    # _pyferret._put will throw an Exception if there is a problem
    _pyferret._put(codename, titlename, fdata, bdfval, data_unit, dset_str,
                  axis_types, axis_names, axis_units, axis_coords)
    return None


def stop():
    """
    Shuts down and release all memory used by Ferret.
    After calling this function do not call any Ferret functions except
    start, which will restart Ferret and re-enable the other functions.

    Returns:
        False if Ferret has not been started or has already been stopped
        True otherwise
    """
    return _pyferret._stop()


def ferret_pyfunc():
    """
    A dummy function (which just returns this help message) used to document the
    requirements of python modules used as Ferret external functions (using the
    Ferret command: DEFINE PYFUNC [/NAME=<alias>] <module.name>).  Two methods,
    ferret_init and ferret_compute, must be provided by such a module:


    ferret_init(id)
        Arguments:
            id - Ferret's integer ID of this external function

        Returns a dictionary defining the following keys:
            "numargs":      number of input arguments [1 - 9; required]
            "descript":     string description of the function [required]
            "axes":         4-tuple (X,Y,Z,T) of result grid axis defining values,
                            which are:
                                    AXIS_ABSTRACT:        indexed, ferret_result_limits
                                                          called to define the axis,
                                    AXIS_CUSTOM:          ferret_custom_axes called to
                                                          define the axis,
                                    AXIS_DOES_NOT_EXIST:  does not exist in (normal to)
                                                          the results grid,
                                    AXIS_IMPLIED_BY_ARGS: same as the corresponding axis
                                                          in one or more arguments,
                                    AXIS_REDUCED:         reduced to a single point
                            [optional; default: AXIS_IMPLIED_BY_ARGS for each axis]
            "argnames":     N-tuple of names for the input arguments
                            [optional; default: (A, B, ...)]
            "argdescripts": N-tuple of descriptions for the input arguments
                            [optional; default: no descriptions]
            "argtypes":     N-tuple of FLOAT_ARG or STRING_ARG, indicating whether
                            the input argument is an array of floating-point values
                            or a single string value.
                            [optional; default: FLOAT_ARG for every argument]
            "influences":   N-tuple of 4-tuples of booleans indicating whether the
                            corresponding input argument's (X,Y,Z,T) axis influences
                            the result grid's (X,Y,Z,T) axis.  [optional; default,
                            and when None is given for a 4-tuple: True for every axis]
                      NOTE: If the "influences" value for an axis is True (which is the
                            default), the "axes" value for this axis must be either
                            AXIS_IMPLIED_BY_ARGS (the default) or AXIS_REDUCED.
            "extends":      N-tuple of 4-tuples of pairs of integers.  The n-th tuple,
                            if not None, gives the (X,Y,Z,T) extension pairs for the
                            n-th argument.  An extension pair, if not None, is the
                            number of points extended in the (low,high) indices for
                            that axis of that argument beyond the implied axis of the
                            result grid.  Thus,
                                    (None, (None, None, None, (-1,1)), None)
                            means the T axis of the second argument is extended by two
                            points (low dimension lowered by 1, high dimension raised
                            by 1) beyond the implied axis of the result.
                            [optional; default: no extensions assigned]
                      NOTE: If an "extends" pair is given for an axis, the "axes"
                            value for this axis must be either AXIS_IMPLIED_BY_ARGS
                            (the default).  The "extends" pair more precisely means
                            the axis in the argument, exactly as provided in the
                            Ferret command, is larger by the indicated amount from
                            the implied result grid axis.

        If an exception is raised, Ferret is notified that an error occurred using
        the message of the exception.


    ferret_compute(id, result_array, result_bdf, input_arrays, input_bdfs)
        Arguments:
            id           - Ferret's integer ID of this external function
            result_array - a writeable NumPy float32 ndarray of four dimensions (X,Y,Z,T)
                           to contain the results of this computation.  The shape and
                           strides of this array has been configured so that only (and
                           all) the data points that should be assigned are accessible.
            result_bdf   - a NumPy ndarray of one dimension containing the bad-data-flag
                           value for the result array.
            input_arrays - tuple of read-only NumPy float32 ndarrays of four dimensions
                           (X,Y,Z,T) containing the given input data.  The shape and
                           strides of these array have been configured so that only (and
                           all) the data points that should be accessible are accessible.
            input_bdfs   - a NumPy ndarray of one dimension containing
                           the bad-data-flag values for each of the input arrays.

        Any return value is ignored.

        If an exception is raised, Ferret is notified that an error occurred using
        the message of the exception.


    If the dictionary returned from ferret_init assigned a result axis as AXIS_ABSTRACT,
    then the ferret_result_limits method must also be defined:


    ferret_result_limits(id)
        Arguments:
            id - Ferret's integer ID of this external function

        Returns a (X,Y,Z,T) 4-tuple of either None or (low, high) pairs of integers.
        If an axis was not designated as AXIS_ABSTRACT, None should be given for that axis.
        If an axis was designated as AXIS_ABSTRACT, a (low, high) pair of integers should
        be given, and are used as the low and high Ferret indices for that axis.
        [The indices of the NumPy ndarray to be assigned will be from 0 until (high-low)].

        If an exception is raised, Ferret is notified that an error occurred using
        the message of the exception.


    If the dictionary returned from ferret_init assigned a result axis as AXIS_CUSTOM,
    then the ferret_custom_axes method must also be defined:


    ferret_custom_axes(id)
        Arguments:
            id - Ferret's integer ID of this external function

        Returns a (X,Y,Z,T) 4-tuple of either None or a (low, high, delta, unit_name,
        is_modulo) tuple.  If an axis was not designated as AXIS_CUSTOM, None should be
        given for that axis.  If an axis was designated as AXIS_CUSTOM, a (low, high,
        delta, unit_name, is_modulo) tuple should be given where low and high are the
        "world" coordinates (floating point) limits for the axis, delta is the step
        increments in "world" coordinates, unit_name is a string used in describing the
        "world" coordinates, and is_modulo is either True or False, indicating if this
        is a modulo ("wrapping") coordinate system.

        If an exception is raised, Ferret is notified that an error occurred using
        the message of the exception.

    """
    return ferret_pyfunc.__doc__


def get_axis_coordinates(id, arg, axis):
    """
    Returns the "world" coordinates for an axis of an argument to an external function

    Arguments:
        id: the Ferret id of the external function
        arg: the index (zero based) of the argument (can use ARG1, ARG2, ..., ARG9)
        axis: the index (zero based) of the axis (can use X_AXIS, Y_AXIS, Z_AXIS, T_AXIS)
    Returns:
        a NumPy float64 ndarray containing the "world" coordinates,
        or None if the values cannot be determined at the time this was called
    Raises:
        ValueError if id, arg, or axis is invalid
    """
    # check the id
    try:
        int_id = int(id)
        if int_id < 0:
            raise ValueError
    except:
        raise ValueError("id must be a positive integer value")
    # check the arg index
    try:
        int_arg = int(arg)
        if (int_arg < _pyferret.ARG1) or (int_arg > _pyferret.ARG9):
            raise ValueError
    except:
        raise ValueError("arg must be an integer value in [%d,%d]" % (_pyferret.ARG1,_pyferret.ARG9))
    # check the axis index
    try:
        int_axis = int(axis)
        if (int_axis < _pyferret.X_AXIS) or (int_axis > _pyferret.T_AXIS):
            raise ValueError
    except:
        raise ValueError("axis must be an integer value in [%d,%d]" % (_pyferret.X_AXIS,_pyferret.T_AXIS))
    # make the actual call
    return _pyferret._get_axis_coordinates(int_id, int_arg, int_axis)


def get_axis_box_sizes(id, arg, axis):
    """
    Returns the "box sizes", in "world" coordinate units,
    for an axis of an argument to an external function

    Arguments:
        id: the Ferret id of the external function
        arg: the index (zero based) of the argument (can use ARG1, ARG2, ..., ARG9)
        axis: the index (zero based) of the axis (can use X_AXIS, Y_AXIS, Z_AXIS, T_AXIS)
    Returns:
        a NumPy float32 ndarray containing the "box sizes",
        or None if the values cannot be determined at the time this was called
    Raises:
        ValueError if id, arg, or axis is invalid
    """
    # check the id
    try:
        int_id = int(id)
        if int_id < 0:
            raise ValueError
    except:
        raise ValueError("id must be a positive integer value")
    # check the arg index
    try:
        int_arg = int(arg)
        if (int_arg < _pyferret.ARG1) or (int_arg > _pyferret.ARG9):
            raise ValueError
    except:
        raise ValueError("arg must be an integer value in [%d,%d]" % (_pyferret.ARG1,_pyferret.ARG9))
    # check the axis index
    try:
        int_axis = int(axis)
        if (int_axis < _pyferret.X_AXIS) or (int_axis > _pyferret.T_AXIS):
            raise ValueError
    except:
        raise ValueError("axis must be an integer value in [%d,%d]" % (_pyferret.X_AXIS,_pyferret.T_AXIS))
    # make the actual call
    return _pyferret._get_axis_box_sizes(int_id, int_arg, int_axis)


def get_axis_box_limits(id, arg, axis):
    """
    Returns the "box limits", in "world" coordinate units,
    for an axis of an argument to an external function

    Arguments:
        id: the Ferret id of the external function
        arg: the index (zero based) of the argument (can use ARG1, ARG2, ..., ARG9)
        axis: the index (zero based) of the axis (can use X_AXIS, Y_AXIS, Z_AXIS, T_AXIS)
    Returns:
        a tuple of two NumPy float64 ndarrays containing the low and high "box limits",
        or None if the values cannot be determined at the time this was called
    Raises:
        ValueError if id, arg, or axis is invalid
    """
    # check the id
    try:
        int_id = int(id)
        if int_id < 0:
            raise ValueError
    except:
        raise ValueError("id must be a positive integer value")
    # check the arg index
    try:
        int_arg = int(arg)
        if (int_arg < _pyferret.ARG1) or (int_arg > _pyferret.ARG9):
            raise ValueError
    except:
        raise ValueError("arg must be an integer value in [%d,%d]" % (_pyferret.ARG1,_pyferret.ARG9))
    # check the axis index
    try:
        int_axis = int(axis)
        if (int_axis < _pyferret.X_AXIS) or (int_axis > _pyferret.T_AXIS):
            raise ValueError
    except:
        raise ValueError("axis must be an integer value in [%d,%d]" % (_pyferret.X_AXIS,_pyferret.T_AXIS))
    # make the actual call
    return _pyferret._get_axis_box_limits(int_id, int_arg, int_axis)


def get_axis_info(id, arg, axis):
    """
    Returns information about the axis of an argument to an external function

    Arguments:
        id: the Ferret id of the external function
        arg: the index (zero based) of the argument (can use ARG1, ARG2, ..., ARG9)
        axis: the index (zero based) of the axis (can use X_AXIS, Y_AXIS, Z_AXIS, T_AXIS)
    Returns:
        a dictionary defining the following keys:
            "name": name string for the axis coordinate
            "unit": name string for the axis unit
            "backwards": boolean - reversed axis?
            "modulo": boolean - periodic/wrapping axis?
            "regular": boolean - evenly spaced axis?
            "size": number of coordinates on this axis, or -1 if the value
                    cannot be determined at the time this was called
    Raises:
        ValueError if id, arg, or axis is invalid
    """
    # check the id
    try:
        int_id = int(id)
        if int_id < 0:
            raise ValueError
    except:
        raise ValueError("id must be a positive integer value")
    # check the arg index
    try:
        int_arg = int(arg)
        if (int_arg < _pyferret.ARG1) or (int_arg > _pyferret.ARG9):
            raise ValueError
    except:
        raise ValueError("arg must be an integer value in [%d,%d]" % (_pyferret.ARG1,_pyferret.ARG9))
    # check the axis index
    try:
        int_axis = int(axis)
        if (int_axis < _pyferret.X_AXIS) or (int_axis > _pyferret.T_AXIS):
            raise ValueError
    except:
        raise ValueError("axis must be an integer value in [%d,%d]" % (_pyferret.X_AXIS,_pyferret.T_AXIS))
    # make the actual call
    return _pyferret._get_axis_info(int_id, int_arg, int_axis)

