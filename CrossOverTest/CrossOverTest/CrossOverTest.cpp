// CrossOverTest.cpp : definisce il punto di ingresso dell'applicazione console.
//

//	Compilare in x64 e release a causa del c++ MySQL connector
//	File mysqlcppconn.dll presente nella cartella x64\Release
//	Richiede l'installazione delle librerie boost

#include "stdafx.h"



#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/json.h>

#include "cpprest/containerstream.h"

/* Standard C++ includes */
#include <stdlib.h>
#include <iostream> 

/*
Include directly the different
headers from cppconn/ and mysql_driver.h + mysql_util.h
(and mysql_connection.h). This will reduce your build time!
*/
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>


using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

using namespace std;


#include <codecvt>
#include <string>

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

// Demonstrates how to iterate over a JSON object.
void IterateJSONValue()
{
	// Create a JSON object.
	json::value obj;
	obj[L"key1"] = json::value::boolean(false);
	obj[L"key2"] = json::value::number(44);
	obj[L"key3"] = json::value::number(43.6);
	obj[L"key4"] = json::value::string(U("str"));

	// Loop over each element in the object.
	for (auto iter = obj.as_object().cbegin(); iter != obj.as_object().cend(); ++iter)
	{
		// Make sure to get the value as const reference otherwise you will end up copying
		// the whole JSON value recursively which can be expensive if it is a nested object.
		//const json::value &str = iter->first;
		const utility::string_t &str = iter->first;
		const json::value &v = iter->second;

		// Perform actions here to process each string and value in the JSON object...
		std::wcout << L"String: " << str << L", Value: " << v.serialize() << endl;
	}

	/* Output:
	String: key1, Value: false
	String: key2, Value: 44
	String: key3, Value: 43.6
	String: key4, Value: str
	*/
}

void print_search_results(json::value const & value, sql::Connection * con)
{
	if (!value.is_null())
	{

		utility::stringstream_t stream;
		//json::value v1 = json::value::string(U("Hi"));
		value.serialize(stream);

		std::wofstream outFile("JSON.TXT");
		outFile << stream.rdbuf();

		//	wcout << value.at(i); genera un'eccezione se il valore i-esimo è un array

		wcout << value.at(L"geonames") << endl << endl;

		auto ResultsArray = value.at(L"geonames").as_array();
		for (int i = 0; i < ResultsArray.size(); i++)
		{
			wcout << ResultsArray[i] << endl << endl;
			wcout << ResultsArray[i].at(L"countrycode") << endl << endl;

			sql::PreparedStatement *pstmt;
			/* '?' is the supported placeholder syntax */
			pstmt = con->prepareStatement("INSERT INTO TestColumn1(id, id2) VALUES (?, ?)");
			for (int i = 1; i <= 10; i++) {
				try
				{
					pstmt->setInt(1, i);
					std::wstring s(ResultsArray[i].at(L"countrycode").as_string());

					std::string name = wstring_to_utf8(s);

					pstmt->setString(2, name);
					pstmt->executeUpdate();
				}
				catch (sql::SQLException &e)
				{
					cout << "# ERR: SQLException in " << __FILE__;
					cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
					cout << "# ERR: " << e.what();
					cout << " (MySQL error code: " << e.getErrorCode();
					cout << ", SQLState: " << e.getSQLState() << " )" << endl;
				}
			}
			delete pstmt;

		}

	}
}

void search_and_print(wstring const & searchTerm, int resultsCount, sql::Connection * con)
{

	// IterateJSONValue();
	// system("Pause");

	//http_client client(U("https://ajax.googleapis.com/ajax/services/search/web"));

	//http_client client(U("https://api.github.com/users/mralexgray/repos"));

	http_client client(U("http://api.geonames.org/citiesJSON?north=44.1&south=-9.9&east=-22.4&west=55.2&lang=de&username=demo"));

	// build the query parameters
	//auto query = uri_builder()
	//.append_path(L"/posts/1")
	//.append_query(L"q", searchTerm)
	//.append_query(L"v", L"1.0")
	//.append_query(L"rsz", resultsCount)	

	//.to_string();

	client
		// send the HTTP GET request asynchronous

		//.request(methods::GET, query)				
		.request(methods::GET)

		// continue when the response is available
		.then([](http_response response) -> pplx::task<json::value>
	{
		// if the status is OK extract the body of the response into a JSON value
		// works only when the content type is application\json
		if (response.status_code() == status_codes::OK)
		{
			return response.extract_json();
		}

		// return an empty JSON value
		return pplx::task_from_result(json::value());
	})
		// continue when the JSON value is available
		.then([=](pplx::task<json::value> previousTask)
	{
		// get the JSON value from the task and display content from it
		try
		{
			json::value const & v = previousTask.get();
			print_search_results(v, con);
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
		.wait();
}



int main(void)
{
	cout << endl;
	cout << "Let's have MySQL count from 10 to 1..." << endl;

	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
		sql::PreparedStatement *pstmt;

		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "1234");
		/* Connect to the MySQL test database */
		con->setSchema("mysql");

		stmt = con->createStatement();

		//	Warning: it deletes the database test
		stmt->execute("DROP DATABASE IF EXISTS test");
		stmt->execute("CREATE DATABASE test");
		stmt->execute("USE test");

		stmt->execute("CREATE TABLE TestColumn1(id INT, id2 varchar(100))");
		delete stmt;

		search_and_print(L"marius bancila", 5, con);

		/* Select in ascending order */
		pstmt = con->prepareStatement("SELECT id FROM TestColumn1 ORDER BY id ASC");
		res = pstmt->executeQuery();

		/* Fetch in reverse = descending order! */
		res->afterLast();
		while (res->previous())
			cout << "\t... MySQL counts: " << res->getInt("id") << endl;
		delete res;

		delete pstmt;
		delete con;

	}
	catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}

	cout << endl;


	system("pause");

	return EXIT_SUCCESS;
}