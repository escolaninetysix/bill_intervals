// BillIntervals.cpp : Defines the entry point for the console application.
//
#include <string>
#include <ios>
#include <iostream>
#include <fstream>

/* Note on format of input txt file:
	All of the transaction start dates must occur after the file Start date, else 
	they will be skipped over (HasStarted will never become true).  This applies
	to Monthly and Interval transactions.

The following sample file exhibits all possible input:
09.19.2002
01.01.2004
,
Once
x
09.23.2002
1637.29
CurrentBalance9.23.2002
,
Monthly
x
09.25.2002
-35.00
FastLane
,
Interval
180
10.01.2002
-80.00
CarServiceB

Note that unlimited numbers of transactions of type Once, 
Monthly and Interval can appear in any order.

Note that for the dollar amount, a '-' minus sign can be 
placed as the first character.

The last entry for each transaction, the Description, can't have whitespace.

Once is a transaction which occurs once at the specified date.
Monthly is a transaction which occurs on the day of the month
 upon which it was started (here the 25th of the month).
Interval is a transaction which occurs once every n days,
 here n = 180 days.
The first two dates at the top of the file are the begin and end
 dates over which transactions will be projected.

The intended use of this program is focused on projecting the
 immediate billings at the day the program is run based on
 completed updated start dates for each transaction:
  --- the start date of each interval transaction needs to
    be calibrated at each run of the program ---  For example,
 car maintenance appointments depend on mileage used, which varies;
 therefore, update the start date of the interval transactions
 for car service to accurately specify the expected next service day.


 */


struct OnceLink
{
	short StartM, StartD, StartY;
	double Amount;
	char Description[100];
	struct OnceLink* NextOnce;
};

struct IntervalLink
{
	short StartM, StartD, StartY, Interval, HasStarted, Counter;
	double Amount;
	char Description[100];
	struct IntervalLink* NextInterval;
};

struct MonthlyLink
{
	short StartM, StartD, StartY, HasStarted;
	double Amount;
	char Description[100];
	struct MonthlyLink* NextMonthly;
};

struct BimonthlyLink
{
	short StartM, StartD, StartY, HasStarted;
	double Amount;
	char Description[100];
	struct BimonthlyLink* NextBimonthly;
};

int StringToNums(char* line, short& month, short& day,
					short& year);

int StringToNumber(char* line, short& number);

int StringToDollars(char* , short&, short& , short& );

int GetBills(short&, short&, short&,
				short&, short&, short&,
				OnceLink*&, IntervalLink*&, MonthlyLink*&, BimonthlyLink*&);

void AdvanceDate(short& month, short& day, short& year);

void PrintTransaction(double Amount, short Month, short Day, 
					  short Year, char* Description, std::ofstream& BillsOut); 

void AddInAllOnce(OnceLink* headOnce, double& Balance, short CurrM, 
				   short CurrD, short CurrY, std::ofstream& BillsOut );

void AddInAllInterval(IntervalLink* headInterval, double& Balance,
					  short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut);

void AddInAllMonthly(MonthlyLink* headMonthly, double& Balance,
					 short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut);

void AddInAllBimonthly(BimonthlyLink* headMonthly, double& Balance,
					 short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut);

// BillIntervals.cpp : Defines the entry point for the console application.
//

int main(int argc, char* argv[])
{
	int nResult;

	struct OnceLink* headOnce = NULL;
	struct IntervalLink* headInterval = NULL;
	struct MonthlyLink* headMonthly = NULL;
	struct BimonthlyLink* headBimonthly = NULL;
	short BeginM, BeginD, BeginY, EndM, EndD, EndY;
	short CurrM, CurrD, CurrY;
	int done;
	double Balance;

	nResult=GetBills(BeginM, BeginD, BeginY,
				EndM, EndD, EndY,
				headOnce, headInterval, headMonthly, headBimonthly);
	if (nResult==-1)
	{
		std::cout << "Couldn't read data file.\n";
		return -1;
	}

	CurrM=BeginM;
	CurrD=BeginD;
	CurrY=BeginY;

	done = 0;

	Balance = 0.0;

	std::ofstream BillsOut = std::ofstream( "Bills.csv", std::ios::out);

	while (!done)
	{
		// do day's work:
		AddInAllOnce(headOnce, Balance, CurrM, CurrD, CurrY, BillsOut);
		AddInAllInterval(headInterval, Balance, CurrM, CurrD, CurrY, BillsOut);
		AddInAllMonthly(headMonthly, Balance, CurrM, CurrD, CurrY, BillsOut);
		AddInAllBimonthly(headBimonthly, Balance, CurrM, CurrD, CurrY, BillsOut);
		// advance to see if another day:
		AdvanceDate(CurrM, CurrD, CurrY);
		if (CurrM==EndM && CurrD == EndD && CurrY == EndY) done=1;
	}
	
	BillsOut.close();

	std::cout << "End balance: " 
		<< Balance 
		<< std::endl;

	return 0;
}

int StringToNums(char* line, short& month, short& day,
					short& year)
{
	
	if (line[0] > 57 || line[0] < 48   // > '9' or < '0'
		|| line[1] > 57 || line[1] < 48
		|| line[2] != 46 // '.'
		|| line[3] > 57 || line[3] < 48
		|| line[4] > 57 || line[4] < 48
		|| line[5] != 46
		|| line[6] > 57 || line[6] < 48
		|| line[7] > 57 || line[7] < 48
		|| line[8] > 57 || line[8] < 48
		|| line[9] > 57 || line[9] < 48
		) 
	{
		std::cout << "Couldn't read the date '"<<line<<"'.  Use format mm.dd.yyyy\n";
		return (-1);
	}
	else  // valid looking date, so return it:
	{
		month = 10*(line[0]-48)+(line[1]-48);
		day = 10*(line[3]-48)+(line[4]-48);
		year = 1000*(line[6]-48)+100*(line[7]-48)+10*(line[8]-48)+(line[9]-48);
	}

	return 0;
}

int StringToNumber(char* line, short& number)
{
	int count=0;  //Put number of digits into count;
	while (line[count]!='\0') count++;
	number = 0;

	int i;

	for (i=0;i<count;i++)
	{
		if (line[i]< 48 
			|| line[i]> 57)
		{
			std::cout << "Number in input '" << line << "' contained non-number characters.\n";
			return -1;
		}
		number=number*10+(line[i]-48);
		//std::cout << "Dollars intermediate:"<<dollars<<std::endl;
	}
	return 0;
}

int StringToDollars(char* line, short& minus, short& dollars, short& cents)
{
	int i, done, dotpos;
	done = 0;
	dollars=0;
	cents=0;
	i=0;
//	std::cout << "toDollars line:" << line << std::endl;
	minus = 0;
	if (line[0]=='-')
	{
		minus = 1;
	}
	while (i <100 && !done)
	{
		if (line[i]=='.')
		{
			done=1;
		}	
		else
		{
			i++;
		}
	}
	if (done!=1)
	{
		std::cout << "Couldn't use line '" << line << "'" << std::endl << " - not in the dollar/cent format d.cc, dd.cc, ddd.cc, etc.\n";
		return -1;
	}
	else
	{
		dotpos=i;
		//std::cout << "dotpos set to:" << dotpos << std::endl;
	}
	for (i=(0+1*(minus==1));i<dotpos;i++)
	{
		if (line[i]< 48 
			|| line[i]> 57)
		{
			std::cout << "Couldn't use line '" << line << "'" << std::endl << " - not in the dollar/cent format d.cc, dd.cc, ddd.cc, etc.\n";
			return -1;
		}
		dollars=dollars*10+(line[i]-48);
		//std::cout << "Dollars intermediate:"<<dollars<<std::endl;
	}
	if (line[dotpos+1]< 48 || line[dotpos+1]>57
		|| line[dotpos+2]< 48 || line[dotpos+2]>57
		|| line[dotpos+3]!='\0'
		)
	{
		std::cout << "Couldn't use line '" << line << "'" << std::endl << " - not in the dollar/cent format d.cc, dd.cc, ddd.cc, etc.\n";
		return -1;
	}
	cents=10*(line[dotpos+1]-48)+(line[dotpos+2]-48);
//	std::cout << minus << ", " << dollars << ", " << cents << std::endl;
	return 0;
}

int GetBills(short&BeginM, short&BeginD, short&BeginY,
				short&EndM, short&EndD, short&EndY,
				OnceLink*& headOnce, 
			  IntervalLink*& headInterval,
			  MonthlyLink*& headMonthly, BimonthlyLink*& headBimonthly)
{
	int nResult, done;

	std::ifstream BillMakerFile = std::ifstream( "BillMakerList.txt", std::ios::in);
	
	// read in the listing's beginning date:
	char line[100];
	char line1[100];
	char line2[100];
	char line3[100];
	char line4[100];
	char line5[100];

	BillMakerFile >> line;
	nResult=StringToNums(line, BeginM, BeginD, BeginY);
	if (nResult==-1)
	{
		std::cout << "Couldn't read Begin date for listing\n";
		BillMakerFile.close();
		return (-1);
	}
	std::cout << "Start: " << BeginM << "/" << BeginD << "/" << BeginY << std::endl;

	BillMakerFile >> line;
	nResult=StringToNums(line, EndM, EndD, EndY);
	if (nResult==-1)
	{
		std::cout << "Couldn't read the first date\n";
		BillMakerFile.close();
		return (-1);
	}
	std::cout << "End: " << EndM << "/" << EndD << "/" << EndY << std::endl;

	done = 0;

	while (!done)
	{	
		short Month, Day, Year, Minus, Dollars, Cents, Interval;

		if (	   !(BillMakerFile >> line)  // read ","
				|| !(BillMakerFile >> line1)
				|| !(BillMakerFile >> line2)
				|| !(BillMakerFile >> line3)
				|| !(BillMakerFile >> line4)
				|| !(BillMakerFile >> line5))
		{
			done = 1;
		}
		else
		{
			//parse the 5 lines into the correct list:
			if (strcmp("Once", line1)==0)
			{	
				//std::cout << "read " << line1 << std::endl;

				// parse data
				nResult= StringToNums(line3, Month, Day, Year);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToDollars(line4, Minus, Dollars, Cents);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				// line5 can be copied into Description as is.

				// insert node at head of list
				struct OnceLink* lastheadOnce = headOnce;
				headOnce = new struct OnceLink;
				headOnce->NextOnce=lastheadOnce;
				lastheadOnce=NULL;
				

				// populate node
				headOnce->StartM = Month;
				headOnce->StartD = Day;
				headOnce->StartY = Year;
				headOnce->Amount = (1.0-2.0*(Minus==1))*
					((double)Dollars+( (double)Cents /100.0));
				strcpy_s(headOnce->Description,99, line5);
				
			}
			else if (strcmp("Interval", line1)==0)
			{
				//std::cout << "read " << line1<< std::endl;

				// parse data
				nResult= StringToNumber(line2, Interval);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToNums(line3, Month, Day, Year);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToDollars(line4, Minus, Dollars, Cents);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				// line5 can be copied into Description as is.

				// Check that Start date of Interval is on or after Begin date
				//    else would be skipped
				if (  (Year< BeginY)
					|| (Year==BeginY && Month < BeginM)
					|| (Year==BeginY && Month==BeginM && Day<BeginD)
					)
				{
					std::cout << "Start date of '" << line5 << 
					"' occurs before report Begin date, please make it on or after report Begin date."
						<< std::endl;
					BillMakerFile.close();
					return -1;
				}


				// insert node at head of list
				struct IntervalLink* lastheadInterval = headInterval;
				headInterval = new struct IntervalLink;
				headInterval->NextInterval=lastheadInterval;
				lastheadInterval=NULL;
				

				// populate node
				headInterval->Interval=Interval;
				headInterval->HasStarted=0;
				headInterval->Counter=0;
				headInterval->StartM = Month;
				headInterval->StartD = Day;
				headInterval->StartY = Year;
				headInterval->Amount = (1.0-2.0*(Minus==1))*
					((double)Dollars+( (double)Cents /100.0));
				strcpy_s(headInterval->Description,99, line5);
			}
			else if (strcmp("Monthly", line1)==0)
			{
				//std::cout << "read " << line1<< std::endl;

				// parse data
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToNums(line3, Month, Day, Year);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToDollars(line4, Minus, Dollars, Cents);
				if (nResult==-1)
				{
					BillMakerFile.close();
					return -1;
				}
				// line5 can be copied into Description as is.

				// Check that Start date of Interval is on or after Begin date
				//    else would be skipped
				if (  (Year< BeginY)
					|| (Year==BeginY && Month < BeginM)
					|| (Year==BeginY && Month==BeginM && Day<BeginD)
					)
				{
					std::cout << "Start date of '" << line5 << 
					"' occurs before report Begin date, please make it on or after report Begin date."
						<< std::endl;
					BillMakerFile.close();
					return -1;
				}

				// insert node at head of list
				struct MonthlyLink* lastheadMonthly = headMonthly;
				headMonthly = new struct MonthlyLink;
				headMonthly->NextMonthly=lastheadMonthly;
				lastheadMonthly=NULL;
				

				// populate node
				headMonthly->StartM = Month;
				headMonthly->StartD = Day;
				headMonthly->StartY = Year;
				headMonthly->Amount = (1.0-2.0*(Minus==1))*
					((double)Dollars+( (double)Cents /100.0));
				strcpy_s(headMonthly->Description,99, line5);
				headMonthly->HasStarted=0;
			}

			else if (strcmp("Bimonthly", line1)==0)
			{
				//std::cout << "read " << line1<< std::endl;

				// parse data
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToNums(line3, Month, Day, Year);
				if (nResult==-1) 
				{
					BillMakerFile.close();
					return -1;
				}
				nResult= StringToDollars(line4, Minus, Dollars, Cents);
				if (nResult==-1)
				{
					BillMakerFile.close();
					return -1;
				}
				// line5 can be copied into Description as is.

				// Check that Start date is on or after Begin date
				//    else would be skipped
				if (  (Year< BeginY)
					|| (Year==BeginY && Month < BeginM)
					|| (Year==BeginY && Month==BeginM && Day<BeginD)
					)
				{
					std::cout << "Start date of '" << line5 << 
					"' occurs before report Begin date, please make it on or after report Begin date."
						<< std::endl;
					BillMakerFile.close();
					return -1;
				}

				// insert node at head of list
				struct BimonthlyLink* lastheadBimonthly = headBimonthly;
				headBimonthly = new struct BimonthlyLink;
				headBimonthly->NextBimonthly=lastheadBimonthly;
				lastheadBimonthly=NULL;
				

				// populate node
				headBimonthly->StartM = Month;
				headBimonthly->StartD = Day;
				headBimonthly->StartY = Year;
				headBimonthly->Amount = (1.0-2.0*(Minus==1))*
					((double)Dollars+( (double)Cents /100.0));
				strcpy_s(headBimonthly->Description,99, line5);
				headBimonthly->HasStarted=0;
			}
			else
			{
				std::cout << "Couldn't read line: " << line1 << "-- aborting file read"<< std::endl;
				BillMakerFile.close();
				return (-1);
			}
		}

	}
	BillMakerFile.close();

	return 0;
}

void AdvanceDate(short& month, short& day, short& year)
{
	int length[12]={31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	++day;
	if (month==2)
	{
		if ( ( (year-2000)%4 ) == 0)
		{
			if (day>29)
			{
				month++;
				day=1;
			}
		}
		else
		{
			if (day>28)
			{
				month++;
				day=1;
			}
		}
	}
	else
	{
		if (day > length[month-1])
		{
			day=1;
			++month;
			if (month>12)
			{
				month=1;
				++year;
			}
		}
	}

	return;
}

void PrintTransaction(double Amount, short Month, short Day, 
		short Year, char* Description, std::ofstream& BillsOut)
{
	BillsOut <<Amount << "," << Month<<"-"<<Day<<"-"<<Year<<","<<Description<<std::endl;
	return;
}


void AddInAllOnce(OnceLink* headOnce, double& Balance,
				  short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut)
{
	OnceLink* currentOnce = headOnce;
	while (currentOnce!=NULL)
	{
		if (currentOnce->StartM==CurrM
			&& currentOnce->StartD==CurrD
			&& currentOnce->StartY==CurrY)
		{
			Balance+=currentOnce->Amount;
			BillsOut << Balance <<",";
			PrintTransaction(currentOnce->Amount,
				CurrM, CurrD, CurrY,
				currentOnce->Description, BillsOut);
		}
		currentOnce=currentOnce->NextOnce;
	}

	return;
}

void AddInAllInterval(IntervalLink* headInterval, double& Balance,
					  short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut)
{
	IntervalLink* currentInterval = headInterval;
	while (currentInterval!=NULL)
	{
		
		if (currentInterval->StartM==CurrM
			&& currentInterval->StartD==CurrD
			&& currentInterval->StartY==CurrY)
		{
			currentInterval->HasStarted=1;
			currentInterval->Counter=0;
		}
		if (currentInterval->HasStarted==1)
		{
			if (currentInterval->Counter==0)
			{
				Balance+=currentInterval->Amount;
				BillsOut << Balance <<",";
				PrintTransaction(currentInterval->Amount,
					CurrM, CurrD, CurrY,
					currentInterval->Description, BillsOut);
			}
			++currentInterval->Counter;
			if (currentInterval->Counter==currentInterval->Interval) 
				currentInterval->Counter=0;
		}
		currentInterval=currentInterval->NextInterval;
	}
	return;
}

void AddInAllMonthly(MonthlyLink* headMonthly, double& Balance,
					 short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut)
{
	MonthlyLink* currentMonthly = headMonthly;
	while (currentMonthly!=NULL)
	{
		if (currentMonthly->HasStarted==0)
		{
			if (currentMonthly->StartM==CurrM
				&& currentMonthly->StartD==CurrD
				&& currentMonthly->StartY==CurrY)
				currentMonthly->HasStarted=1;
		}
		if (currentMonthly->HasStarted==1)
		{
			if (CurrD==currentMonthly->StartD)
			{
				Balance+=currentMonthly->Amount;
				BillsOut << Balance <<",";
				PrintTransaction(currentMonthly->Amount,
					CurrM, CurrD, CurrY,
					currentMonthly->Description, BillsOut);
			}
		}
		currentMonthly=currentMonthly->NextMonthly;
	}
	return;
}

void AddInAllBimonthly(BimonthlyLink* headBimonthly, double& Balance,
					 short CurrM, short CurrD, short CurrY, std::ofstream& BillsOut)
{
	int length[12]={31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	BimonthlyLink* currentBimonthly = headBimonthly;
	while (currentBimonthly!=NULL)
	{
		if (currentBimonthly->HasStarted==0)
		{
			if (currentBimonthly->StartM==CurrM
					&& currentBimonthly->StartD==CurrD
					&& currentBimonthly->StartY==CurrY
				)
				currentBimonthly->HasStarted=1;
		}
		if (currentBimonthly->HasStarted==1)
		{
			if	(
					(CurrD == 15)
					||
					(CurrD == length[CurrM-1])
					
				)
			{
				Balance+=currentBimonthly->Amount;
				BillsOut << Balance <<",";
				PrintTransaction(currentBimonthly->Amount,
					CurrM, CurrD, CurrY,
					currentBimonthly->Description, BillsOut);
			}
		}
		currentBimonthly=currentBimonthly->NextBimonthly;
	}
	return;
}
