#ifndef __QUERYPARSER_H__
#define __QUERYPARSER_H__

#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

class QueryParserResult
{
  private:
    std::string query;
    std::string croppedQuery;
    std::string sortColumn;
    short sortColumnNumber;
    short sortColumnType;
    
  public:
    QueryParserResult();
    QueryParserResult(std::string _query);
    
    std::string getQuery();
    QueryParserResult* setQuery(std::string _query);
    std::string getCroppedQuery();
    QueryParserResult* setCroppedQuery(std::string _croppedQuery);
    std::string getSortColumn();
    QueryParserResult* setSortColumn(std::string _sortColumn);
    short getSortColumnNumber();
    QueryParserResult* setSortColumnNumber(short _sortColumnNumber);
    short getSortColumnType();
    QueryParserResult* setSortColumnType(short _sortColumnType);
};

class QueryParser
{
  private:
    static std::vector<std::string> explodeColumns(const std::string& s, char delim);
  public:
    static QueryParserResult parse(std::string query, bool explicitType, MYSQL *conn);
};

inline QueryParserResult QueryParser::parse(std::string query, bool explicitType, MYSQL *conn)
{
  QueryParserResult result(query);
  
  boost::regex pattern("SELECT(?:\\s+SQL_NO_CACHE)?\\s+(\\*|[\\w\\.,\\s]*?)\\s+FROM.*(ORDER\\s+BY\\s+([\\w\\.]+))\\s*");
  boost::smatch matches;
  
  if (boost::regex_match(query, matches, pattern)) {
    //the column given to order by
    result.setSortColumn(matches[3]);
    
    std::string parsedQuery;

    //ORDER BY ...
    boost::regex orderByPattern(matches[2].str());

    //cut the end of the query
    boost::regex_replace(std::back_inserter(parsedQuery), query.begin(), query.end(), orderByPattern, "");
    result.setCroppedQuery(parsedQuery);

    short i;
    if (matches[1].str() != "*" && explicitType) {
      //iterate over exploded columns and set sortColumnNumber
      std::vector<std::string> columns = QueryParser::explodeColumns(matches[1].str(), ',');
      for (i = 0; i < columns.size(); i++) {
	if (columns[i] == result.getSortColumn()) {
	  result.setSortColumnNumber(i);
	  break;
	}
	
	if (i == columns.size() - 1) {
	  //TODO: throw an exception, or do some error handling
	}
      }
    } else {
      //execute query with LIMIT 1, then iterate over columns, and set sortColumnNumber and sortColumnType
      std::ostringstream limitedQueryBuilder;
      limitedQueryBuilder<<parsedQuery<<" LIMIT 1";
      
      MYSQL_RES *queryResult;
      MYSQL_FIELD* field;

      mysql_query(conn, limitedQueryBuilder.str().c_str());
      queryResult = mysql_store_result(conn);
      
      //tableName.(columnName)
      boost::regex sortColumnPattern("(?:[\\w]*\\.)?([\\w]+)");
      boost::smatch sortColumnMatches;

      i = 0;
      std::ostringstream columnName;
      while (field = mysql_fetch_field(queryResult)) {
	if (boost::regex_match(result.getSortColumn(), sortColumnMatches, sortColumnPattern)) {
	  columnName.str("");
	  columnName<<field->name;
	  if (columnName.str() == sortColumnMatches[1].str()) {
	    result.setSortColumnNumber(i);
	    if (!explicitType) {
	      result.setSortColumnType(field->type);
	    }
	    break;
	  }
	}
	i++;
      }
      
      mysql_free_result(queryResult);
    }    

  }
  
  return result;
}

inline std::vector<std::string> QueryParser::explodeColumns(const std::string& s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);
    
    boost::regex columnPattern("[\\w\\.]+\\s+AS\\s+([\\w\\.]+)");
    boost::smatch matches;
    
    std::string token;

    while (std::getline(iss, token, delim) )
    {
      boost::trim(token);
      
      if (boost::regex_match(token, matches, columnPattern)) {
	token = matches[1];
      }
      
      result.push_back(token);
    }

    return result;
}

inline QueryParserResult::QueryParserResult()
{
  this->query = "";
  this->croppedQuery = "";
  this->sortColumn = "";
  this->sortColumnNumber = 0;
};

inline QueryParserResult::QueryParserResult(std::string _query)
{
  this->query = _query;
  this->croppedQuery = "";
  this->sortColumn = "";
  this->sortColumnNumber = 0;
};

inline std::string QueryParserResult::getQuery()
{
  return this->query;
}

inline QueryParserResult* QueryParserResult::setQuery(std::string _query)
{
  this->query = _query;
  
  return this;
}

inline std::string QueryParserResult::getCroppedQuery()
{
  return this->croppedQuery;
}

inline QueryParserResult* QueryParserResult::setCroppedQuery(std::string _croppedQuery)
{
  this->croppedQuery = _croppedQuery;
  
  return this;
}

inline std::string QueryParserResult::getSortColumn()
{
  return this->sortColumn;
}

inline QueryParserResult* QueryParserResult::setSortColumn(std::string _sortColumn)
{
  boost::trim(_sortColumn);
  this->sortColumn = _sortColumn;
  
  return this;
}

inline short QueryParserResult::getSortColumnNumber()
{
  return this->sortColumnNumber;
}

inline QueryParserResult* QueryParserResult::setSortColumnNumber(short _sortColumnNumber)
{
  this->sortColumnNumber = _sortColumnNumber;
  
  return this;
}

inline short QueryParserResult::getSortColumnType()
{
  return this->sortColumnType;
}

inline QueryParserResult* QueryParserResult::setSortColumnType(short _sortColumnType)
{
  this->sortColumnType = _sortColumnType;
  
  return this;
}

#endif